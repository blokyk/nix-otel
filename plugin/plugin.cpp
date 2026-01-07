#include <algorithm>
#include <dlfcn.h>
#include <iostream>
#include <iterator>
#include <optional>
#include <string_view>

#include <kj/async.h>

#include <lix/config.h>
#include <lix/libutil/config.hh>
#include <lix/libutil/logging.hh>
//#include <lix/libutil/configuration.hh>
#include <lix/libexpr/eval-inline.hh>
#include <lix/libstore/globals.hh>
#include <lix/libexpr/primops.hh>

#if NIX_HAVE_BOEHMGC

#include <gc/gc.h>
#include <gc/gc_cpp.h>

#endif

#include "./nix_otel_plugin.h"

using namespace nix;

extern "C" void discourage_linker_from_discarding() {}

static auto marshalActivityType(ActivityType at) -> ActivityKind {
  switch (at) {
  case actCopyPath:
    return ActivityKind::CopyPath;
  case actFileTransfer:
    return ActivityKind::FileTransfer;
  case actRealise:
    return ActivityKind::Realise;
  case actCopyPaths:
    return ActivityKind::CopyPaths;
  case actBuilds:
    return ActivityKind::Builds;
  case actBuild:
    return ActivityKind::Build;
  case actOptimiseStore:
    return ActivityKind::OptimiseStore;
  case actVerifyPaths:
    return ActivityKind::VerifyPaths;
  case actSubstitute:
    return ActivityKind::Substitute;
  case actQueryPathInfo:
    return ActivityKind::QueryPathInfo;
  case actPostBuildHook:
    return ActivityKind::PostBuildHook;
  case actBuildWaiting:
    return ActivityKind::BuildWaiting;
  default:
  case actUnknown:
    return ActivityKind::Unknown;
  }
}

static auto marshalResultType(ResultType rt) -> ResultKind {
  switch (rt) {
  case resFileLinked:
    return ResultKind::FileLinked;
  case resBuildLogLine:
    return ResultKind::BuildLogLine;
  case resUntrustedPath:
    return ResultKind::UntrustedPath;
  case resCorruptedPath:
    return ResultKind::CorruptedPath;
  case resSetPhase:
    return ResultKind::SetPhase;
  case resProgress:
    return ResultKind::Progress;
  case resSetExpected:
    return ResultKind::SetExpected;
  case resPostBuildLogLine:
    return ResultKind::PostBuildLogLine;
  default:
    return ResultKind::Unknown;
  }
}

static auto marshalField(Logger::Field const &field) -> FfiField {
  if (field.type == nix::Logger::Field::tInt) {
    return FfiField{
        .tag = FfiField::Tag::Num,
        .num = {field.i},
    };
  } else if (field.type == nix::Logger::Field::tString) {
    return FfiField{
        .tag = FfiField::Tag::String,
        .string = {FfiString{.start = field.s.data(), .len = field.s.length()}},
    };
  }
  // w/e
  __builtin_abort();
}

static auto marshalFields(Logger::Fields const &fields)
    -> std::vector<FfiField> {
  std::vector<FfiField> out{};
  for (auto &&f : fields) {
    out.push_back(marshalField(f));
  }

  return out;
}

static auto unwrapVectorToFfiFields(std::vector<FfiField> const &fields_)
    -> FfiFields {
  return FfiFields{.start = fields_.data(), .count = fields_.size()};
}

class OTelLogger : public Logger {
private:
  Context const *m_context;

public:
  Logger *upstream;

  OTelLogger(Logger *upstream, Context const *context)
      : m_context(context), upstream(upstream) {}
  ~OTelLogger() = default;

  void pause() override { upstream->pause(); }
  void resetProgress() override { upstream->resetProgress(); }
  void resume() override { upstream->resume(); }

  bool isVerbose() override { return upstream->isVerbose(); }

  BufferState bufferState() const override {
    if (upstream->bufferState() == BufferState::NeedsFlush)
      return BufferState::NeedsFlush;

    return BufferState::HasSpace;
  }

  BufferState log(Verbosity lvl, std::string_view fs) override {
    return upstream->log(lvl, fs);
  }

  BufferState logEI(const ErrorInfo &ei) override {
    return upstream->logEI(ei);
  }

  BufferState startActivityImpl(ActivityId act, Verbosity lvl, ActivityType type,
                     const std::string &s, const Fields &fields,
                     ActivityId parent) override {
    auto fields_ = marshalFields(fields);
    start_activity(m_context, act, marshalActivityType(type), s.c_str(),
                   unwrapVectorToFfiFields(fields_), parent);
    return upstream->startActivity(act, lvl, type, s, fields, parent);
  };

  BufferState stopActivityImpl(ActivityId act) override {
    end_activity(m_context, act);
    return upstream->stopActivity(act);
  };

  BufferState resultImpl(ActivityId act, ResultType type, const Fields &fields) override {
    auto fields_ = marshalFields(fields);
    on_result(m_context, act, marshalResultType(type),
              unwrapVectorToFfiFields(fields_));
    return upstream->result(act, type, fields);
  };

  void writeToStdout(std::string_view s) override {
    upstream->writeToStdout(s);
  }

  std::optional<char> ask(std::string_view s) override {
    return upstream->ask(s);
  }

  void waitForSpace(NeverAsync = {}) override { upstream->waitForSpace(); }

  kj::Promise<Result<void>> flush() override {
      return upstream->flush();
  }
};

class PluginInstance {
  Context *context;

public:
  PluginInstance() {
    context = initialize_plugin();
    logger = new OTelLogger(logger, context);
  }

  ~PluginInstance() {
    // get the OTelLogger wrapper we setup, and then set
    // the global logger to the one that it wrapped around,
    // which ensures that we don't "leave any trace" and
    // the runtime is back to its pre-plugin form.
    // (note: this also assumes that nothing wrapped the
    // logger after we did or that it unwrapped it before
    // we get destroyed...)
    auto toDestroy = logger;
    logger = (dynamic_cast<OTelLogger*>(toDestroy))->upstream;

    deinitialize_plugin(context);

    delete toDestroy;
  }
};

PluginInstance x{};

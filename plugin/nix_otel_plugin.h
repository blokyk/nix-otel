#pragma once

enum class ActivityKind {
  CopyPath,
  FileTransfer,
  Realise,
  CopyPaths,
  Builds,
  Build,
  OptimiseStore,
  VerifyPaths,
  Substitute,
  QueryPathInfo,
  PostBuildHook,
  BuildWaiting,
  Unknown,
};

enum class ResultKind {
  FileLinked,
  BuildLogLine,
  UntrustedPath,
  CorruptedPath,
  SetPhase,
  Progress,
  SetExpected,
  PostBuildLogLine,
  Unknown,
};

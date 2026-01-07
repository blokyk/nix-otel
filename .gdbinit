set env OTEL_EXPORTER_OTLP_ENDPOINT http://localhost:4317
set args "--option" "plugin-files" "./target/x86_64-unknown-linux-gnu/release/libnix_otel_plugin.so" "--builders" "" "--expr" 'with import <nixpkgs> {}; callPackage ./. {}'
set auto-load safe-path $debugdir:$datadir/auto-load:/nix/store

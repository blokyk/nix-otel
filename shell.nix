{
  lib,
  callPackage,
  enableDebugging,

  lix,
  makeWrapper,
  nix,

  nix-otel-plugin ?
    (callPackage ./default.nix {})
      .overrideAttrs (_: { pname = "nix-otel-plugin"; }),

  symlinkJoin,
  writeShellApplication,
}: {
  nix-build-otel = writeShellApplication {
    name = "nix-build-otel";
    runtimeInputs = [ (enableDebugging nix) nix-otel-plugin ];
    text = ''
      nix-build --option plugin-files ${nix-otel-plugin}/lib/libnix_otel_plugin.so "''${@}"
    '';
  };

  nix-with-otel = symlinkJoin {
    name = "nix-with-otel";
    paths = [ (enableDebugging nix) nix-otel-plugin ];
    buildInputs = [ makeWrapper ];
    postBuild = ''
      wrapProgram $out/bin/nix \
        --add-flag "--option" \
        --add-flag "plugin-files" \
        --add-flag "$out/lib/libnix_otel_plugin.so"
    '';
  };
}

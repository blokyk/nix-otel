{
  boost,
  capnproto,
  rustPlatform,
  buildRustPackage ? rustPlatform.buildRustPackage,
  lib,

  nix,
  debugNix ? (enableDebugging (nix.overrideAttrs (_: { dontStrip = true; }))),

  lix,
  debugLix ? (enableDebugging (lix.overrideAttrs (_: { dontStrip = true; }))),

  pkg-config,
  protobuf,

  enableDebugging,

  bear,
  rust-cbindgen,
}: buildRustPackage (finalAttrs: {
  pname = "nix-otel-plugin";
  version = "2023-08-06";

  src = ./.; /* fetchFromGitHub {
    owner = "blokyk";
    repo = "nix-otel";
    rev = "25894595a8cbe1a09c93547d5bb839f439ee94bf";
    sha256 = "sha256-FWHVCw/mT07nzm1g/PBFDuttA3XJr6OzgIIJy7uIwEI=";
  };*/

  cargoLock.lockFile = "${finalAttrs.src}/Cargo.lock";

  nativeBuildInputs = [
    pkg-config
    protobuf
    nix
    # debugLix

    bear
    rust-cbindgen
  ];

  buildInputs = [
    boost
    nix
    # debugLix
    capnproto
  ];

  dontStrip = true;

  meta = {
    description = "Fork of lf-'s 'nix-otel', a Nix OpenTelemetry sender plugin.";
    homepage = "https://github.com/blokyk/nix-otel";
    license = lib.licenses.mit;
    platform = lib.platforms.linux;
    maintainers = with lib.maintainers; [ blokyk ];
  };
})

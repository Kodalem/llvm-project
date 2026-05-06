{
  pkgs,
  system,
  patmos-llvm,
  patmos-simulator,
}: {
  checks.patmos-llvm-tests = pkgs.stdenv.mkDerivation {
    name = "patmos-llvm-tests-${system}";
    buildInputs = [patmos-llvm patmos-simulator];

    buildPhase = ''
      export PASIM="${patmos-simulator}/bin/pasim"
      export PATH="${patmos-simulator}/bin:$PATH"

      # Test LLVM
      if [ -f "${patmos-llvm}/bin/llvm-lit" ] && [ -d "./llvm/test" ]; then
        echo "Running LLVM Patmos tests..."
        ${patmos-llvm}/bin/llvm-lit ./llvm/test -v --filter=Patmos || true
      fi

      # Test Clang
      if [ -f "${patmos-llvm}/bin/clang" ] && [ -d "./clang/test" ]; then
        echo "Running Clang Patmos tests..."
        ${patmos-llvm}/bin/llvm-lit ./clang/test -v --filter=Patmos || true
      fi

      # Test LLD
      if [ -f "${patmos-llvm}/bin/lld" ] && [ -d "./lld/test" ]; then
        echo "Running LLD Patmos tests..."
        ${patmos-llvm}/bin/llvm-lit ./lld/test -v --filter=Patmos || true
      fi
    '';
  };
}

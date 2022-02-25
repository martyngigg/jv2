{
  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-21.05";
    flake-utils.url = "github:numtide/flake-utils";
    flake-utils.inputs.nixpkgs.follows = "nixpkgs";
    bundler.url = "github:matthewbauer/nix-bundle";
    bundler.inputs.nixpkgs.follows = "nixpkgs";
    nixGL-src.url = "github:guibou/nixGL";
    nixGL-src.flake = false;
    weggli.url = "github:googleprojectzero/weggli";
    weggli.flake = false;
  };
  outputs = { self, nixpkgs, flake-utils, bundler, nixGL-src, weggli }:
    let
      exe-name = mpi: gui:
        if mpi then
          "jv2-mpi"
        else
          (if gui then "jv2" else "jv2");
      cmake-bool = x: if x then "ON" else "OFF";
      version = "0.1";
      base_libs = pkgs:
        with pkgs; [
          antlr4
          antlr4.runtime.cpp
          cmake
          cli11
          fmt
          fmt.dev
          freetype
          inetutils # for rsh
          jre
          pkgconfig
          pugixml
        ];
      gui_libs = pkgs:
        with pkgs; [
          freetype
          ftgl
          libGL.dev
          libglvnd
          libglvnd.dev
        ];
      check_libs = pkgs: with pkgs; [ gtest ];

    in flake-utils.lib.eachSystem [ "x86_64-linux" ] (system:

      let
        pkgs = nixpkgs.legacyPackages.${system};
        nixGL = import nixGL-src { inherit pkgs; };
        QTDIR = "${import ./nix/qt6.nix { inherit pkgs; }}/6.1.1/gcc_64";
        jv2 =
          { mpi ? false, gui ? true, threading ? true, checks ? false }:
          assert (!(gui && mpi));
          pkgs.gcc9Stdenv.mkDerivation ({
            inherit version;
            pname = exe-name mpi gui;
            src =
              builtins.filterSource (path: type: baseNameOf path != "flake.nix")
              ./.;
            patches = [ ./nix/patches/ctest.patch ];
            buildInputs = base_libs pkgs ++ pkgs.lib.optional mpi pkgs.openmpi
              ++ pkgs.lib.optionals gui (gui_libs pkgs)
              ++ pkgs.lib.optionals checks (check_libs pkgs)
              ++ pkgs.lib.optional threading pkgs.tbb;

            TBB_DIR = "${pkgs.tbb}";
            CTEST_OUTPUT_ON_FAILURE = "ON";

            cmakeFlags = [
              "-DCONAN=OFF"
              ("-DMULTI_THREADING=" + (cmake-bool threading))
              ("-DPARALLEL=" + (cmake-bool mpi))
              ("-DGUI=" + (cmake-bool gui))
              "-DBUILD_SYSTEM_TESTS:bool=${cmake-bool checks}"
              "-DBUILD_UNIT_TESTS:bool=${cmake-bool (checks && !mpi)}"
              ("-DCMAKE_BUILD_TYPE=" + (if checks then "Debug" else "Release"))
            ] ++ pkgs.lib.optional threading
              ("-DTHREADING_LINK_LIBS=${pkgs.tbb}/lib/libtbb.so");
            doCheck = checks;
            installPhase = ''
              mkdir -p $out/bin
              ls -R
              cp -R ./jv2 $out/bin/
              
            '';

            meta = with pkgs.lib; {
              description = "";
              homepage = "";
              # license = licenses.unlicense;
              maintainers = [ maintainers.rprospero ];
            };
          } // (if true then {
            inherit QTDIR;
            Qt6_DIR = "${QTDIR}/lib/cmake/Qt6";
            Qt6CoreTools_DIR = "${QTDIR}/lib/cmake/Qt6CoreTools";
            Qt6GuiTools_DIR = "${QTDIR}/lib/cmake/Qt6GuiTools";
            Qt6WidgetsTools_DIR = "${QTDIR}/lib/cmake/Qt6WidgetsTools";
            Qt6ChartsTools_DIR = "${QTDIR}/lib/cmake/Qt6ChartsTools";
            Qt6XmlTools_DIR = "${QTDIR}/lib/cmake/Qt6XmlTools";

          } else
            { }))
          // (if checks then { QT_QPA_PLATFORM = "offscreen"; } else { });
        mkSingularity = { mpi ? false, gui ? false, threading ? true }:
          pkgs.singularity-tools.buildImage {
            name = "${exe-name mpi gui}-${version}";
            diskSize = 1024 * 25;
            contents = [ (jv2 { inherit mpi gui threading; }) ];
            runScript = if true then
              "${nixGL.nixGLIntel}/bin/nixGLIntel ${
                jv2 { inherit mpi gui threading; }
              }/bin/${exe-name mpi gui}"
            else
              "${jv2 { inherit mpi gui threading; }}/bin/${
                exe-name mpi gui
              }";
          };
      in {
        checks.jv2 = jv2 { checks = true; };
        checks.jv2-mpi = jv2 {
          mpi = true;
          gui = true;
          checks = true;
        };
        checks.jv2-threadless = jv2 {
          threading = false;
          gui = true;
          checks = true;
        };

        defaultPackage = self.packages.${system}.jv2-gui;

        devShell = pkgs.gcc9Stdenv.mkDerivation {
          name = "jv2-shell";
          buildInputs = base_libs pkgs ++ gui_libs pkgs ++ check_libs pkgs
            ++ (with pkgs; [
              (pkgs.clang-tools.override { llvmPackages = pkgs.llvmPackages; })
              ccache
              ccls
              cmake-format
              cmake-language-server
              conan
              distcc
              gdb
              ninja
              openmpi
              tbb
              valgrind
              ((import ./nix/weggli.nix) {
                inherit pkgs;
                src = weggli;
              })
            ]);
          CMAKE_CXX_COMPILER_LAUNCHER = "${pkgs.ccache}/bin/ccache";
          CMAKE_CXX_FLAGS_DEBUG = "-g -O0";
          CXXL = "${pkgs.stdenv.cc.cc.lib}";
          inherit QTDIR;
          Qt6_DIR = "${QTDIR}/lib/cmake/Qt6";
          Qt6CoreTools_DIR = "${QTDIR}/lib/cmake/Qt6CoreTools";
          Qt6GuiTools_DIR = "${QTDIR}/lib/cmake/Qt6GuiTools";
          Qt6WidgetsTools_DIR = "${QTDIR}/lib/cmake/Qt6WidgetsTools";
          Qt6ChartsTools_DIR = "${QTDIR}/lib/cmake/Qt6ChartsTools";
          Qt6XmlTools_DIR = "${QTDIR}/lib/cmake/Qt6XmlTools";
          PATH = "${QTDIR}/bin";
          THREADING_LINK_LIBS = "${pkgs.tbb}/lib/libtbb.so";
        };

        apps = {
          jv2 =
            flake-utils.lib.mkApp { drv = self.packages.${system}.jv2; };
          jv2-mpi = flake-utils.lib.mkApp {
            drv = self.packages.${system}.jv2-mpi;
          };
          jv2-gui = flake-utils.lib.mkApp {
            drv = self.packages.${system}.jv2-gui;
          };
        };

        defaultApp =
          flake-utils.lib.mkApp { drv = self.defaultPackage.${system}; };

        packages = {
          jv2 = jv2 { gui = true; };
          jv2-threadless = jv2 {
            gui = true;
            threading = false;
          };
          jv2-mpi = jv2 {
            mpi = true;
            gui = true;
          };
          jv2-gui = jv2 { };

          singularity = mkSingularity { };
          singularity-mpi = mkSingularity { mpi = true; };
          singularity-gui = mkSingularity { gui = true; };
          singularity-threadless = mkSingularity {
            gui = true;
            threading = false;
          };

          docker = pkgs.dockerTools.buildImage {
            name = "jv2";
            tag = "latest";
            config.Cmd = [ "${self.packages.${system}.jv2}/bin/jv2" ];
          };

          docker-gui = pkgs.dockerTools.buildImage {
            name = "jv2";
            tag = "latest";
            config.ENTRYPOINT = [
              "${nixGL.nixGLIntel}/bin/nixGLIntel"
              "${self.packages.${system}.jv2}/bin/jv2"
            ];
          };

          docker-mpi = pkgs.dockerTools.buildImage {
            name = "jv2-mpi";
            tag = "latest";
            config.ENTRYPOINT =
              [ "${self.packages.${system}.jv2-mpi}/bin/jv2-mpi" ];
          };

        };
      });
}

# Save the target triple in a variable
execute_process( COMMAND gcc -dumpmachine OUTPUT_VARIABLE DUMP_MACHINE OUTPUT_STRIP_TRAILING_WHITESPACE )
message(STATUS "Machine Triple: ${DUMP_MACHINE}")
if (${DUMP_MACHINE} MATCHES "x86_64-linux-gnu")
	set( TARGET_TRIPLE "x86_64-linux-gnu")
elseif(${DUMP_MACHINE} MATCHES "x86_64-apple-darwin.*")
	set( TARGET_TRIPLE "x86_64-apple-darwin")
elseif(${DUMP_MACHINE} MATCHES "arm64-apple-darwin.*")
	set( TARGET_TRIPLE "arm64-apple-darwin")
else()
	message(FATAL_ERROR "Unsupported platform for packaging")
endif()
set( PROJECT_NAME "patmos-llvm")
set( PATMOS_TRIPLE "patmos-unknown-unknown-elf" )
set( PACKAGE_TEMP_DIR "${PATMOS_TRIPLE}/package-temp")
set( PACKAGE_TAR "${PACKAGE_TEMP_DIR}/${PROJECT_NAME}-${TARGET_TRIPLE}.tar")
set( PACKAGE_TAR_GZ "${PACKAGE_TAR}.gz")
set( PACKAGE_INFO_FILE "${PACKAGE_TEMP_DIR}/${PROJECT_NAME}-info.yml")
set( PACKAGE_TARGETS "llc" "llvm-link" "clang" "llvm-config" "llvm-objdump" "opt" "lld")
set( PACKAGE_ITEMS_LIBS
	"${PACKAGE_TEMP_DIR}/${PATMOS_TRIPLE}/lib/crt0.o"
	"${PACKAGE_TEMP_DIR}/${PATMOS_TRIPLE}/lib/crtbegin.o"
	"${PACKAGE_TEMP_DIR}/${PATMOS_TRIPLE}/lib/crtend.o"
	"${PACKAGE_TEMP_DIR}/${PATMOS_TRIPLE}/lib/libc.a"
	"${PACKAGE_TEMP_DIR}/${PATMOS_TRIPLE}/lib/libm.a"
	"${PACKAGE_TEMP_DIR}/${PATMOS_TRIPLE}/lib/libpatmos.a"
	"${PACKAGE_TEMP_DIR}/${PATMOS_TRIPLE}/lib/librt.a"
	"${PACKAGE_TEMP_DIR}/${PATMOS_TRIPLE}/lib/libsyms.o"
)
set( NEWLIB_INCLUDES # List of dirs from which to get only the files
	"newlib/libc/include"
	"newlib/libc/include/machine"
	"newlib/libc/include/rpc"
	"newlib/libc/include/sys"
	"newlib/libc/machine/patmos/machine" # -> include/machine
	"newlib/libc/machine/patmos/sys" # -> include/sys
	"newlib/libc/machine/patmos/include" # -> include
)
set( MOVED_BINARIES
	"${PACKAGE_TEMP_DIR}/bin/patmos-llc" 
	"${PACKAGE_TEMP_DIR}/bin/patmos-llvm-link" 
	"${PACKAGE_TEMP_DIR}/bin/patmos-clang-12" 
	"${PACKAGE_TEMP_DIR}/bin/patmos-llvm-config" 
	"${PACKAGE_TEMP_DIR}/bin/patmos-llvm-objdump" 
	"${PACKAGE_TEMP_DIR}/bin/patmos-opt" 
	"${PACKAGE_TEMP_DIR}/bin/patmos-lld"
)
set( PACKAGE_ITEMS
	# Package binaries
	${MOVED_BINARIES}
	"${PACKAGE_TEMP_DIR}/bin/patmos-clang" # Actually a symlink
	"${PACKAGE_TEMP_DIR}/bin/patmos-ld.lld" # Actually a symlink

	# Clang headers
	"${PACKAGE_TEMP_DIR}/lib/clang" 
	
	# Std headers
	"${PACKAGE_TEMP_DIR}/${PATMOS_TRIPLE}/include"
	
	${PACKAGE_ITEMS_LIBS}
)

add_custom_target(PatmosPackageTempDirs 
    "${CMAKE_COMMAND}" -E make_directory 
	"${PACKAGE_TEMP_DIR}/bin" 
	"${PACKAGE_TEMP_DIR}/lib" 
	"${PACKAGE_TEMP_DIR}/lib/clang/12.0.1/include" 
	"${PACKAGE_TEMP_DIR}/${PATMOS_TRIPLE}/lib"
	"${PACKAGE_TEMP_DIR}/${PATMOS_TRIPLE}/include"
)
add_custom_command(
	OUTPUT ${MOVED_BINARIES} "${PACKAGE_TEMP_DIR}/lib/clang" 
	
	COMMAND cp "${CMAKE_BINARY_DIR}/bin/llc" "${PACKAGE_TEMP_DIR}/bin/patmos-llc"
	COMMAND cp "${CMAKE_BINARY_DIR}/bin/llvm-link" "${PACKAGE_TEMP_DIR}/bin/patmos-llvm-link"
	COMMAND cp "${CMAKE_BINARY_DIR}/bin/clang-12" "${PACKAGE_TEMP_DIR}/bin/patmos-clang-12"
	COMMAND cp "${CMAKE_BINARY_DIR}/bin/llvm-config" "${PACKAGE_TEMP_DIR}/bin/patmos-llvm-config"
	COMMAND cp "${CMAKE_BINARY_DIR}/bin/llvm-objdump" "${PACKAGE_TEMP_DIR}/bin/patmos-llvm-objdump"
	COMMAND cp "${CMAKE_BINARY_DIR}/bin/opt" "${PACKAGE_TEMP_DIR}/bin/patmos-opt"
	COMMAND cp "${CMAKE_BINARY_DIR}/bin/lld" "${PACKAGE_TEMP_DIR}/bin/patmos-lld"

	DEPENDS 
		PatmosPackageTempDirs 
		"${CMAKE_BINARY_DIR}/bin/llc" "${CMAKE_BINARY_DIR}/bin/llvm-link" "${CMAKE_BINARY_DIR}/bin/clang-12" 
		"${CMAKE_BINARY_DIR}/bin/llvm-config" "${CMAKE_BINARY_DIR}/bin/llvm-objdump" "${CMAKE_BINARY_DIR}/bin/opt" 
		"${CMAKE_BINARY_DIR}/bin/lld" 
)
add_custom_command(
	OUTPUT ${PACKAGE_ITEMS_LIBS}
	COMMAND cp "../build-newlib/${PATMOS_TRIPLE}/libgloss/patmos/crt0.o" "${PACKAGE_TEMP_DIR}/${PATMOS_TRIPLE}/lib/"
	COMMAND cp "../build-newlib/${PATMOS_TRIPLE}/libgloss/patmos/crtbegin.o" "${PACKAGE_TEMP_DIR}/${PATMOS_TRIPLE}/lib/"
	COMMAND cp "../build-newlib/${PATMOS_TRIPLE}/libgloss/patmos/crtend.o" "${PACKAGE_TEMP_DIR}/${PATMOS_TRIPLE}/lib/"	
	COMMAND cp "../build-newlib/${PATMOS_TRIPLE}/libgloss/patmos/libpatmos.a" "${PACKAGE_TEMP_DIR}/${PATMOS_TRIPLE}/lib/"
	COMMAND cp "../build-newlib/${PATMOS_TRIPLE}/newlib/libc/libc.a" "${PACKAGE_TEMP_DIR}/${PATMOS_TRIPLE}/lib/"
	COMMAND cp "../build-newlib/${PATMOS_TRIPLE}/newlib/libm/libm.a" "${PACKAGE_TEMP_DIR}/${PATMOS_TRIPLE}/lib/"
	COMMAND cp "../build-compiler-rt/lib/generic/libclang_rt.builtins-patmos.a" "${PACKAGE_TEMP_DIR}/${PATMOS_TRIPLE}/lib/librt.a"
	COMMAND bin/clang -c -emit-llvm "${CMAKE_CURRENT_LIST_DIR}/patmos-libsyms.ll" -o "${PACKAGE_TEMP_DIR}/${PATMOS_TRIPLE}/lib/libsyms.o" -Wno-override-module
	DEPENDS 
		PatmosPackageTempDirs 
		"${CMAKE_CURRENT_LIST_DIR}/patmos-libsyms.ll"
		"../build-newlib/${PATMOS_TRIPLE}/libgloss/patmos/crt0.o" 
		"../build-newlib/${PATMOS_TRIPLE}/libgloss/patmos/crtbegin.o"
		"../build-newlib/${PATMOS_TRIPLE}/libgloss/patmos/crtend.o"
		"../build-newlib/${PATMOS_TRIPLE}/libgloss/patmos/libpatmos.a"
		"../build-newlib/${PATMOS_TRIPLE}/newlib/libc/libc.a"
		"../build-newlib/${PATMOS_TRIPLE}/newlib/libm/libm.a"
		"../build-compiler-rt/lib/generic/libclang_rt.builtins-patmos.a"
)
add_custom_command(
	OUTPUT "${PACKAGE_TEMP_DIR}/lib/clang/12.0.1/include/stdint.h" 	# We need a target, but don't want to define all the files
	
	COMMAND rsync "${CMAKE_BINARY_DIR}/lib/clang/12.0.1/include/*" "${PACKAGE_TEMP_DIR}/lib/clang/12.0.1/include/"
	
	DEPENDS 
		PatmosPackageTempDirs "${CMAKE_BINARY_DIR}/lib/clang/12.0.1/include/stdint.h"
)
add_custom_command(
	OUTPUT "${PACKAGE_TEMP_DIR}/${PATMOS_TRIPLE}/include/newlib.h" # We need a target, but don't want to define all the files
	COMMAND rsync "../patmos-newlib/newlib/libc/include/*" "${PACKAGE_TEMP_DIR}/${PATMOS_TRIPLE}/include/"
	COMMAND rsync "../patmos-newlib/newlib/libc/include/machine/*" "${PACKAGE_TEMP_DIR}/${PATMOS_TRIPLE}/include/machine/"
	COMMAND rsync "../patmos-newlib/newlib/libc/include/rpc/*" "${PACKAGE_TEMP_DIR}/${PATMOS_TRIPLE}/include/rpc/"
	COMMAND rsync "../patmos-newlib/newlib/libc/include/sys/*" "${PACKAGE_TEMP_DIR}/${PATMOS_TRIPLE}/include/sys/"
	COMMAND rsync "../patmos-newlib/newlib/libc/machine/patmos/machine/*" "${PACKAGE_TEMP_DIR}/${PATMOS_TRIPLE}/include/machine/"
	COMMAND rsync "../patmos-newlib/newlib/libc/machine/patmos/sys/*" "${PACKAGE_TEMP_DIR}/${PATMOS_TRIPLE}/include/sys/"
	COMMAND rsync "../patmos-newlib/newlib/libc/machine/patmos/include/*" "${PACKAGE_TEMP_DIR}/${PATMOS_TRIPLE}/include/"
)

ADD_CUSTOM_TARGET(symlink-clang-lld
	COMMAND ${CMAKE_COMMAND} -E create_symlink patmos-clang-12 ${PACKAGE_TEMP_DIR}/bin/patmos-clang
	COMMAND ${CMAKE_COMMAND} -E create_symlink patmos-lld ${PACKAGE_TEMP_DIR}/bin/patmos-ld.lld
	DEPENDS clang PatmosPackageTempDirs		  
	)	
STRING(REGEX REPLACE "${PACKAGE_TEMP_DIR}/" ";"  PACKAGE_ITEMS_IN_TAR ${PACKAGE_ITEMS})
STRING(REGEX REPLACE "${PACKAGE_TEMP_DIR}/" ";"  PACKAGE_INFO_FILE_IN_TAR ${PACKAGE_INFO_FILE})
# Build release tarball containing binaries and metadata
add_custom_command(
	OUTPUT ${PACKAGE_TAR_GZ} 
		# We add all generated files to the list of outputs to ensure that cleaning will remove them.
		${PACKAGE_TAR} ${PACKAGE_INFO_FILE} 

	# Package binaries
	COMMAND tar -cf ${PACKAGE_TAR} -C ${PACKAGE_TEMP_DIR}/ ${PACKAGE_ITEMS_IN_TAR}

	# Build YAML info file
	COMMAND ${CMAKE_COMMAND} -E echo "name: ${PROJECT_NAME}" > ${PACKAGE_INFO_FILE}
	COMMAND ${CMAKE_COMMAND} -E echo "target: ${TARGET_TRIPLE}" >> ${PACKAGE_INFO_FILE}
	COMMAND ${CMAKE_COMMAND} -E echo_append "version: " >> ${PACKAGE_INFO_FILE}
	COMMAND git describe --tags --always >> ${PACKAGE_INFO_FILE}
	COMMAND ${CMAKE_COMMAND} -E echo_append "commit: " >> ${PACKAGE_INFO_FILE}
	COMMAND git rev-parse HEAD >> ${PACKAGE_INFO_FILE}
	COMMAND ${CMAKE_COMMAND} -E echo "files:\\| " >> ${PACKAGE_INFO_FILE}
	COMMAND tar -tf ${PACKAGE_TAR} | sed "s/^/  /" >> ${PACKAGE_INFO_FILE}
	
	# Add the info file to the package
	COMMAND tar -rf ${PACKAGE_TAR} -C ${PACKAGE_TEMP_DIR}/ ${PACKAGE_INFO_FILE_IN_TAR}
	
	# Compress the tar
	COMMAND gzip -9 < ${PACKAGE_TAR} > ${PACKAGE_TAR_GZ}
	
	DEPENDS 
		${PACKAGE_TARGETS} ${PACKAGE_ITEMS_LIBS} ${MOVED_BINARIES} symlink-clang-lld 
		"${PACKAGE_TEMP_DIR}/lib/clang/12.0.1/include/stdint.h" 
		"${PACKAGE_TEMP_DIR}/${PATMOS_TRIPLE}/include/newlib.h" 
)
# Rename release tarball target to something better.
add_custom_target(PatmosPackage DEPENDS ${PACKAGE_TAR_GZ})
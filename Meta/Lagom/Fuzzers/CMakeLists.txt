function(add_simple_fuzzer name)
  add_executable(${name} "${name}.cpp")

  if (ENABLE_FUZZERS_OSSFUZZ)
      target_link_libraries(${name}
          PUBLIC ${ARGN} LibCore)
  elseif (ENABLE_FUZZERS_LIBFUZZER)
    target_compile_options(${name}
      PRIVATE $<$<CXX_COMPILER_ID:Clang>:-g -O1 -fsanitize=fuzzer>
      )
    target_link_libraries(${name}
      PUBLIC ${ARGN} LibCore
      PRIVATE $<$<CXX_COMPILER_ID:Clang>:-fsanitize=fuzzer>
      )
  else()
    target_sources(${name} PRIVATE "EntryShim.cpp")
    target_link_libraries(${name} PUBLIC ${ARGN} LibCore)
  endif()
endfunction()

add_simple_fuzzer(FuzzBMPLoader LibGfx)
add_simple_fuzzer(FuzzBrotli LibCompress)
if (ENABLE_LAGOM_LIBWEB)
    add_simple_fuzzer(FuzzCSSParser LibWeb)
endif()
add_simple_fuzzer(FuzzCyrillicDecoder LibTextCodec)
add_simple_fuzzer(FuzzDeflateCompression LibCompress)
add_simple_fuzzer(FuzzDeflateDecompression LibCompress)
add_simple_fuzzer(FuzzELF LibELF)
add_simple_fuzzer(FuzzFlacLoader LibAudio)
add_simple_fuzzer(FuzzGemini LibGemini)
add_simple_fuzzer(FuzzGIFLoader LibGfx)
add_simple_fuzzer(FuzzGzipCompression LibCompress)
add_simple_fuzzer(FuzzGzipDecompression LibCompress)
add_simple_fuzzer(FuzzICCProfile LibGfx)
add_simple_fuzzer(FuzzICOLoader LibGfx)
add_simple_fuzzer(FuzzJPEGLoader LibGfx)
add_simple_fuzzer(FuzzLzmaDecompression LibArchive LibCompress)
add_simple_fuzzer(FuzzMatroskaReader LibVideo)
add_simple_fuzzer(FuzzMD5 LibCrypto)
add_simple_fuzzer(FuzzMP3Loader LibAudio)
add_simple_fuzzer(FuzzPEM LibCrypto)
add_simple_fuzzer(FuzzPNGLoader LibGfx)
add_simple_fuzzer(FuzzPBMLoader LibGfx)
add_simple_fuzzer(FuzzPGMLoader LibGfx)
add_simple_fuzzer(FuzzPoly1305 LibCrypto)
add_simple_fuzzer(FuzzPPMLoader LibGfx)
add_simple_fuzzer(FuzzPDF LibPDF)
add_simple_fuzzer(FuzzQOALoader LibAudio)
add_simple_fuzzer(FuzzQOILoader LibGfx)
add_simple_fuzzer(FuzzTGALoader LibGfx)
add_simple_fuzzer(FuzzQuotedPrintableParser LibIMAP)
add_simple_fuzzer(FuzzHebrewDecoder LibTextCodec)
add_simple_fuzzer(FuzzHttpRequest LibHTTP)
add_simple_fuzzer(FuzzIMAPParser LibIMAP)
add_simple_fuzzer(FuzzJs LibJS)
add_simple_fuzzer(FuzzLatin1Decoder LibTextCodec)
add_simple_fuzzer(FuzzLatin2Decoder LibTextCodec)
add_simple_fuzzer(FuzzMarkdown LibMarkdown)
add_simple_fuzzer(FuzzRegexECMA262 LibRegex)
add_simple_fuzzer(FuzzRegexPosixBasic LibRegex)
add_simple_fuzzer(FuzzRegexPosixExtended LibRegex)
add_simple_fuzzer(FuzzASN1 LibCrypto LibTLS)
add_simple_fuzzer(FuzzSHA1 LibCrypto)
add_simple_fuzzer(FuzzSHA256 LibCrypto)
add_simple_fuzzer(FuzzSHA384 LibCrypto)
add_simple_fuzzer(FuzzSHA512 LibCrypto)
add_simple_fuzzer(FuzzShell LibShell)
add_simple_fuzzer(FuzzShellPosix LibShell)
add_simple_fuzzer(FuzzSQLParser LibSQL)
add_simple_fuzzer(FuzzTar LibArchive)
add_simple_fuzzer(FuzzTTF LibGfx)
add_simple_fuzzer(FuzzURL)
add_simple_fuzzer(FuzzUTF16BEDecoder LibTextCodec)
add_simple_fuzzer(FuzzRSAKeyParsing LibCrypto)
add_simple_fuzzer(FuzzVP9Decoder LibVideo)
add_simple_fuzzer(FuzzWAVLoader LibAudio)
add_simple_fuzzer(FuzzWasmParser LibWasm)
add_simple_fuzzer(FuzzWOFF LibGfx)
add_simple_fuzzer(FuzzXML LibXML)
add_simple_fuzzer(FuzzZip LibArchive)
add_simple_fuzzer(FuzzZlibDecompression LibCompress)

if (ENABLE_FUZZERS_LIBFUZZER)
set(CMAKE_EXE_LINKER_FLAGS "${ORIGINAL_CMAKE_EXE_LINKER_FLAGS} -fsanitize=address")
set(CMAKE_SHARED_LINKER_FLAGS "${ORIGINAL_CMAKE_SHARED_LINKER_FLAGS} -fsanitize=address")
set(CMAKE_MODULE_LINKER_FLAGS "${ORIGINAL_CMAKE_MODULE_LINKER_FLAGS} -fsanitize=address")
add_executable(FuzzilliJs FuzzilliJs.cpp)
target_compile_options(FuzzilliJs
    PRIVATE $<$<CXX_COMPILER_ID:Clang>:-g -O1 -fsanitize-coverage=trace-pc-guard>
    )
target_link_libraries(FuzzilliJs
    PUBLIC LibCore LibJS
    PRIVATE $<$<CXX_COMPILER_ID:Clang>:-fsanitize-coverage=trace-pc-guard>
    )
endif()

cmake_minimum_required(VERSION 3.12)
project(7zip C CXX ASM_MASM RC)

set(USE_LTO YES CACHE BOOL "Use LTO/LTCG")
set(ENABLE_NSIS_DECOMPILE NO CACHE BOOL "Enable NSIS script decompiling")

set(COMMON_DIR "CPP/Common")
set(WIN_DIR "CPP/Windows")
set(7ZIP_COMMON_DIR "CPP/7zip/Common")
set(AR_DIR "CPP/7zip/Archive")
set(AR_COMMON_DIR "CPP/7zip/Archive/Common")
set(COMPRESS_DIR "CPP/7zip/Compress")
set(CRYPTO_DIR "CPP/7zip/Crypto")
set(UI_DIR "CPP/7zip/UI ")
set(C_DIR "C")

include(CheckIPOSupported)
check_ipo_supported(RESULT ipo_supported OUTPUT error)

add_definitions(-DUNICODE -D_UNICODE)

if(MSVC OR "${CLANG_CXX_COMPILER}" MATCHES "clang-cl")
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Gy -Gw")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Gy -Gw")
  set(CMAKE_EXE_LINKER_FLAGS    "${CMAKE_EXE_LINKER_FLAGS} /MANIFEST:NO")
  set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} /MANIFEST:NO")
  set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} /MANIFEST:NO")
endif()

if("${CMAKE_CXX_COMPILER}" MATCHES "clang-cl")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-c++11-narrowing")
endif()

if(CMAKE_SIZEOF_VOID_P EQUAL 8)
  add_definitions(-Dx64)
  add_definitions(-D_LZMA_DEC_OPT)
  set(LZMA_DEC_FILES Asm/x86/LzmaDecOpt.asm)
else()
  if(MSVC)
    set(CMAKE_EXE_LINKER_FLAGS    "${CMAKE_EXE_LINKER_FLAGS} /LARGEADDRESSAWARE")
    set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} /LARGEADDRESSAWARE")
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} /LARGEADDRESSAWARE")
  else()
    set(CMAKE_EXE_LINKER_FLAGS    "${CMAKE_EXE_LINKER_FLAGS} -Wl,--large-address-aware")
    set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} -Wl,--large-address-aware")
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,--large-address-aware")
  endif()
endif()

if(USE_LTO AND ipo_supported)
  message(STATUS "Using LTO")
  set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
else()
  message(STATUS "Not using LTO")
endif()

if(ENABLE_NSIS_DECOMPILE)
  add_definitions(-DNSIS_SCRIPT)
endif()

add_library(CRC OBJECT Asm/x86/7zCrcOpt.asm ${C_DIR}/7zCrc.c)

add_library(LzFindOpt OBJECT Asm/x86/LzFindOpt.asm)


add_library(Sha1 OBJECT
  Asm/x86/Sha1Opt.asm
  ${COMMON_DIR}/Sha1Prepare.cpp
  ${C_DIR}/Sha1.c
)

add_library(Sha256 OBJECT
  Asm/x86/Sha256Opt.asm
  ${COMMON_DIR}/Sha256Prepare.cpp
  ${C_DIR}/Sha256.c
)

add_library(Aes OBJECT Asm/x86/AesOpt.asm ${C_DIR}/Aes.c)

add_library(COMMON OBJECT
  ${COMMON_DIR}/IntToString.cpp
  ${COMMON_DIR}/MyString.cpp
  ${COMMON_DIR}/MyVector.cpp
  ${COMMON_DIR}/NewHandler.cpp
  ${COMMON_DIR}/StringConvert.cpp
  ${COMMON_DIR}/StringToInt.cpp
  ${COMMON_DIR}/UTFConvert.cpp
  ${COMMON_DIR}/Wildcard.cpp
)


# add_library(Format7z)
# set_target_properties(Format7z PROPERTIES OUTPUT_NAME "7za")

# add_library(Format7zExtract)
# set_target_properties(Format7zExtract PROPERTIES OUTPUT_NAME "7zxa")

# add_library(Format7zR)
# set_target_properties(Format7zR PROPERTIES OUTPUT_NAME "7zr")

# add_library(Format7zExtractR)
# set_target_properties(Format7zExtractR PROPERTIES OUTPUT_NAME "7zxr")

add_library(Format7zF MODULE
  $<TARGET_OBJECTS:COMMON>
  ${COMMON_DIR}/CRC.cpp
  ${COMMON_DIR}/CrcReg.cpp
  ${COMMON_DIR}/DynLimBuf.cpp
  ${COMMON_DIR}/LzFindPrepare.cpp
  ${COMMON_DIR}/MyMap.cpp
  ${COMMON_DIR}/MyXml.cpp
  ${COMMON_DIR}/Sha1Reg.cpp
  ${COMMON_DIR}/Sha256Reg.cpp
  ${COMMON_DIR}/XzCrc64Init.cpp
  ${COMMON_DIR}/XzCrc64Reg.cpp
  ${WIN_DIR}/FileDir.cpp
  ${WIN_DIR}/FileFind.cpp
  ${WIN_DIR}/FileIO.cpp
  ${WIN_DIR}/FileName.cpp
  ${WIN_DIR}/PropVariant.cpp
  ${WIN_DIR}/PropVariantConv.cpp
  ${WIN_DIR}/PropVariantUtils.cpp
  ${WIN_DIR}/Synchronization.cpp
  ${WIN_DIR}/System.cpp
  ${WIN_DIR}/TimeUtils.cpp
  ${7ZIP_COMMON_DIR}/CreateCoder.cpp
  ${7ZIP_COMMON_DIR}/CWrappers.cpp
  ${7ZIP_COMMON_DIR}/InBuffer.cpp
  ${7ZIP_COMMON_DIR}/InOutTempBuffer.cpp
  ${7ZIP_COMMON_DIR}/FilterCoder.cpp
  ${7ZIP_COMMON_DIR}/LimitedStreams.cpp
  ${7ZIP_COMMON_DIR}/LockedStream.cpp
  ${7ZIP_COMMON_DIR}/MemBlocks.cpp
  ${7ZIP_COMMON_DIR}/MethodId.cpp
  ${7ZIP_COMMON_DIR}/MethodProps.cpp
  ${7ZIP_COMMON_DIR}/OffsetStream.cpp
  ${7ZIP_COMMON_DIR}/OutBuffer.cpp
  ${7ZIP_COMMON_DIR}/OutMemStream.cpp
  ${7ZIP_COMMON_DIR}/ProgressMt.cpp
  ${7ZIP_COMMON_DIR}/ProgressUtils.cpp
  ${7ZIP_COMMON_DIR}/PropId.cpp
  ${7ZIP_COMMON_DIR}/StreamBinder.cpp
  ${7ZIP_COMMON_DIR}/StreamObjects.cpp
  ${7ZIP_COMMON_DIR}/StreamUtils.cpp
  ${7ZIP_COMMON_DIR}/UniqBlocks.cpp
  ${7ZIP_COMMON_DIR}/VirtThread.cpp
  ${AR_DIR}/ApmHandler.cpp
  ${AR_DIR}/ApfsHandler.cpp
  ${AR_DIR}/ArHandler.cpp
  ${AR_DIR}/ArjHandler.cpp
  ${AR_DIR}/Base64Handler.cpp
  ${AR_DIR}/Bz2Handler.cpp
  ${AR_DIR}/ComHandler.cpp
  ${AR_DIR}/CpioHandler.cpp
  ${AR_DIR}/CramfsHandler.cpp
  ${AR_DIR}/DeflateProps.cpp
  ${AR_DIR}/DmgHandler.cpp
  ${AR_DIR}/ElfHandler.cpp
  ${AR_DIR}/ExtHandler.cpp
  ${AR_DIR}/FatHandler.cpp
  ${AR_DIR}/FlvHandler.cpp
  ${AR_DIR}/GzHandler.cpp
  ${AR_DIR}/GptHandler.cpp
  ${AR_DIR}/HandlerCont.cpp
  ${AR_DIR}/HfsHandler.cpp
  ${AR_DIR}/IhexHandler.cpp
  ${AR_DIR}/LpHandler.cpp
  ${AR_DIR}/LzhHandler.cpp
  ${AR_DIR}/LzmaHandler.cpp
  ${AR_DIR}/MachoHandler.cpp
  ${AR_DIR}/MbrHandler.cpp
  ${AR_DIR}/MslzHandler.cpp
  ${AR_DIR}/MubHandler.cpp
  ${AR_DIR}/NtfsHandler.cpp
  ${AR_DIR}/PeHandler.cpp
  ${AR_DIR}/PpmdHandler.cpp
  ${AR_DIR}/QcowHandler.cpp
  ${AR_DIR}/RpmHandler.cpp
  ${AR_DIR}/SparseHandler.cpp
  ${AR_DIR}/SplitHandler.cpp
  ${AR_DIR}/SquashfsHandler.cpp
  ${AR_DIR}/SwfHandler.cpp
  ${AR_DIR}/UefiHandler.cpp
  ${AR_DIR}/VdiHandler.cpp
  ${AR_DIR}/VhdHandler.cpp
  ${AR_DIR}/VhdxHandler.cpp
  ${AR_DIR}/VmdkHandler.cpp
  ${AR_DIR}/XarHandler.cpp
  ${AR_DIR}/XzHandler.cpp
  ${AR_DIR}/ZHandler.cpp
  ${AR_DIR}/Common/CoderMixer2.cpp
  ${AR_DIR}/Common/DummyOutStream.cpp
  ${AR_DIR}/Common/FindSignature.cpp
  ${AR_DIR}/Common/InStreamWithCRC.cpp
  ${AR_DIR}/Common/ItemNameUtils.cpp
  ${AR_DIR}/Common/MultiStream.cpp
  ${AR_DIR}/Common/OutStreamWithCRC.cpp
  ${AR_DIR}/Common/OutStreamWithSha1.cpp
  ${AR_DIR}/Common/HandlerOut.cpp
  ${AR_DIR}/Common/ParseProperties.cpp
  ${AR_DIR}/7z/7zCompressionMode.cpp
  ${AR_DIR}/7z/7zDecode.cpp
  ${AR_DIR}/7z/7zEncode.cpp
  ${AR_DIR}/7z/7zExtract.cpp
  ${AR_DIR}/7z/7zFolderInStream.cpp
  ${AR_DIR}/7z/7zHandler.cpp
  ${AR_DIR}/7z/7zHandlerOut.cpp
  ${AR_DIR}/7z/7zHeader.cpp
  ${AR_DIR}/7z/7zIn.cpp
  ${AR_DIR}/7z/7zOut.cpp
  ${AR_DIR}/7z/7zProperties.cpp
  ${AR_DIR}/7z/7zSpecStream.cpp
  ${AR_DIR}/7z/7zUpdate.cpp
  ${AR_DIR}/7z/7zRegister.cpp
  ${AR_DIR}/Cab/CabBlockInStream.cpp
  ${AR_DIR}/Cab/CabHandler.cpp
  ${AR_DIR}/Cab/CabHeader.cpp
  ${AR_DIR}/Cab/CabIn.cpp
  ${AR_DIR}/Cab/CabRegister.cpp
  ${AR_DIR}/Chm/ChmHandler.cpp
  ${AR_DIR}/Chm/ChmIn.cpp
  ${AR_DIR}/Iso/IsoHandler.cpp
  ${AR_DIR}/Iso/IsoHeader.cpp
  ${AR_DIR}/Iso/IsoIn.cpp
  ${AR_DIR}/Iso/IsoRegister.cpp
  ${AR_DIR}/Nsis/NsisDecode.cpp
  ${AR_DIR}/Nsis/NsisHandler.cpp
  ${AR_DIR}/Nsis/NsisIn.cpp
  ${AR_DIR}/Nsis/NsisRegister.cpp
  ${AR_DIR}/Rar/RarHandler.cpp
  ${AR_DIR}/Rar/Rar5Handler.cpp
  ${AR_DIR}/Tar/TarHandler.cpp
  ${AR_DIR}/Tar/TarHandlerOut.cpp
  ${AR_DIR}/Tar/TarHeader.cpp
  ${AR_DIR}/Tar/TarIn.cpp
  ${AR_DIR}/Tar/TarOut.cpp
  ${AR_DIR}/Tar/TarUpdate.cpp
  ${AR_DIR}/Tar/TarRegister.cpp
  ${AR_DIR}/Udf/UdfHandler.cpp
  ${AR_DIR}/Udf/UdfIn.cpp
  ${AR_DIR}/Wim/WimHandler.cpp
  ${AR_DIR}/Wim/WimHandlerOut.cpp
  ${AR_DIR}/Wim/WimIn.cpp
  ${AR_DIR}/Wim/WimRegister.cpp
  ${AR_DIR}/Zip/ZipAddCommon.cpp
  ${AR_DIR}/Zip/ZipHandler.cpp
  ${AR_DIR}/Zip/ZipHandlerOut.cpp
  ${AR_DIR}/Zip/ZipIn.cpp
  ${AR_DIR}/Zip/ZipItem.cpp
  ${AR_DIR}/Zip/ZipOut.cpp
  ${AR_DIR}/Zip/ZipUpdate.cpp
  ${AR_DIR}/Zip/ZipRegister.cpp
  ${COMPRESS_DIR}/Bcj2Coder.cpp
  ${COMPRESS_DIR}/Bcj2Register.cpp
  ${COMPRESS_DIR}/BcjCoder.cpp
  ${COMPRESS_DIR}/BcjRegister.cpp
  ${COMPRESS_DIR}/BitlDecoder.cpp
  ${COMPRESS_DIR}/BranchMisc.cpp
  ${COMPRESS_DIR}/BranchRegister.cpp
  ${COMPRESS_DIR}/ByteSwap.cpp
  ${COMPRESS_DIR}/BZip2Crc.cpp
  ${COMPRESS_DIR}/BZip2Decoder.cpp
  ${COMPRESS_DIR}/BZip2Encoder.cpp
  ${COMPRESS_DIR}/BZip2Register.cpp
  ${COMPRESS_DIR}/CopyCoder.cpp
  ${COMPRESS_DIR}/CopyRegister.cpp
  ${COMPRESS_DIR}/Deflate64Register.cpp
  ${COMPRESS_DIR}/DeflateDecoder.cpp
  ${COMPRESS_DIR}/DeflateEncoder.cpp
  ${COMPRESS_DIR}/DeflateRegister.cpp
  ${COMPRESS_DIR}/DeltaFilter.cpp
  ${COMPRESS_DIR}/ImplodeDecoder.cpp
  ${COMPRESS_DIR}/LzfseDecoder.cpp
  ${COMPRESS_DIR}/LzhDecoder.cpp
  ${COMPRESS_DIR}/Lzma2Decoder.cpp
  ${COMPRESS_DIR}/Lzma2Encoder.cpp
  ${COMPRESS_DIR}/Lzma2Register.cpp
  ${COMPRESS_DIR}/LzmaDecoder.cpp
  ${COMPRESS_DIR}/LzmaEncoder.cpp
  ${COMPRESS_DIR}/LzmaRegister.cpp
  ${COMPRESS_DIR}/LzmsDecoder.cpp
  ${COMPRESS_DIR}/LzOutWindow.cpp
  ${COMPRESS_DIR}/LzxDecoder.cpp
  ${COMPRESS_DIR}/PpmdDecoder.cpp
  ${COMPRESS_DIR}/PpmdEncoder.cpp
  ${COMPRESS_DIR}/PpmdRegister.cpp
  ${COMPRESS_DIR}/PpmdZip.cpp
  ${COMPRESS_DIR}/QuantumDecoder.cpp
  ${COMPRESS_DIR}/Rar1Decoder.cpp
  ${COMPRESS_DIR}/Rar2Decoder.cpp
  ${COMPRESS_DIR}/Rar3Decoder.cpp
  ${COMPRESS_DIR}/Rar3Vm.cpp
  ${COMPRESS_DIR}/Rar5Decoder.cpp
  ${COMPRESS_DIR}/RarCodecsRegister.cpp
  ${COMPRESS_DIR}/ShrinkDecoder.cpp
  ${COMPRESS_DIR}/XpressDecoder.cpp
  ${COMPRESS_DIR}/XzDecoder.cpp
  ${COMPRESS_DIR}/XzEncoder.cpp
  ${COMPRESS_DIR}/ZlibDecoder.cpp
  ${COMPRESS_DIR}/ZlibEncoder.cpp
  ${COMPRESS_DIR}/ZDecoder.cpp
  ${CRYPTO_DIR}/7zAes.cpp
  ${CRYPTO_DIR}/7zAesRegister.cpp
  ${CRYPTO_DIR}/HmacSha1.cpp
  ${CRYPTO_DIR}/HmacSha256.cpp
  ${CRYPTO_DIR}/MyAes.cpp
  ${CRYPTO_DIR}/MyAesReg.cpp
  ${CRYPTO_DIR}/Pbkdf2HmacSha1.cpp
  ${CRYPTO_DIR}/RandGen.cpp
  ${CRYPTO_DIR}/Rar20Crypto.cpp
  ${CRYPTO_DIR}/Rar5Aes.cpp
  ${CRYPTO_DIR}/RarAes.cpp
  ${CRYPTO_DIR}/WzAes.cpp
  ${CRYPTO_DIR}/ZipCrypto.cpp
  ${CRYPTO_DIR}/ZipStrong.cpp
  ${C_DIR}/7zBuf2.c
  ${C_DIR}/7zStream.c
  ${C_DIR}/Alloc.c
  ${C_DIR}/Bcj2.c
  ${C_DIR}/Bcj2Enc.c
  ${C_DIR}/Blake2s.c
  ${C_DIR}/Bra.c
  ${C_DIR}/Bra86.c
  ${C_DIR}/BraIA64.c
  ${C_DIR}/BwtSort.c
  ${C_DIR}/CpuArch.c
  ${C_DIR}/Delta.c
  ${C_DIR}/HuffEnc.c
  ${C_DIR}/LzFind.c
  ${C_DIR}/LzFindMt.c
  ${C_DIR}/Lzma2Dec.c
  ${C_DIR}/Lzma2DecMt.c
  ${C_DIR}/Lzma2Enc.c
  ${C_DIR}/LzmaDec.c
  ${C_DIR}/LzmaEnc.c
  ${C_DIR}/MtCoder.c
  ${C_DIR}/MtDec.c
  ${C_DIR}/Ppmd7.c
  ${C_DIR}/Ppmd7Dec.c
  ${C_DIR}/Ppmd7aDec.c
  ${C_DIR}/Ppmd7Enc.c
  ${C_DIR}/Ppmd8.c
  ${C_DIR}/Ppmd8Dec.c
  ${C_DIR}/Ppmd8Enc.c
  ${C_DIR}/Sort.c
  ${C_DIR}/Threads.c
  ${C_DIR}/Xz.c
  ${C_DIR}/XzDec.c
  ${C_DIR}/XzEnc.c
  ${C_DIR}/XzIn.c
  ${C_DIR}/XzCrc64.c

  ${COMPRESS_DIR}/CodecExports.cpp
  ${AR_DIR}/ArchiveExports.cpp
  ${AR_DIR}/DllExports2.cpp

  ${AR_DIR}/Archive2.def

  Asm/x86/XzCrc64Opt.asm

  $<TARGET_OBJECTS:CRC>
  $<TARGET_OBJECTS:LzFindOpt>
  $<TARGET_OBJECTS:Sha1>
  $<TARGET_OBJECTS:Sha256>
  $<TARGET_OBJECTS:Aes>

  ${LZMA_DEC_FILES}
  ${LZ_FIND_FILES}

  CPP/7zip/Bundles/Format7zF/resource.rc
  ${AR_DIR}/Archive2.def
)
set_target_properties(Format7zF PROPERTIES OUTPUT_NAME "7z" PREFIX "")
target_compile_definitions(Format7zF PRIVATE -DEXTERNAL_CODECS -D_7ZIP_LARGE_PAGES)

# add_executable(LzmaCon )
# set_target_properties(LzmaCon PROPERTIES OUTPUT_NAME "lzma")

# add_executable(SFXCon )
# set_target_properties(SFXCon PROPERTIES OUTPUT_NAME "7zCon" SUFFIX ".sfx")

# add_executable(SFXWin )
# set_target_properties(SFXWin PROPERTIES OUTPUT_NAME "7z" SUFFIX ".sfx")

# add_executable(SFXSetup )
# set_target_properties(SFXSetup PROPERTIES OUTPUT_NAME "7zS" SUFFIX ".sfx")

add_executable(Console 

  $<TARGET_OBJECTS:COMMON>
  ${COMMON_DIR}/CRC.cpp
  ${COMMON_DIR}/CommandLineParser.cpp
  ${COMMON_DIR}/DynLimBuf.cpp
  ${COMMON_DIR}/ListFileUtils.cpp
  ${COMMON_DIR}/StdInStream.cpp
  ${COMMON_DIR}/StdOutStream.cpp
  
  ${WIN_DIR}/DLL.cpp
  ${WIN_DIR}/ErrorMsg.cpp
  ${WIN_DIR}/FileDir.cpp
  ${WIN_DIR}/FileFind.cpp
  ${WIN_DIR}/FileIO.cpp
  ${WIN_DIR}/FileLink.cpp
  ${WIN_DIR}/FileName.cpp
  ${WIN_DIR}/FileSystem.cpp
  ${WIN_DIR}/MemoryLock.cpp
  ${WIN_DIR}/PropVariant.cpp
  ${WIN_DIR}/PropVariantConv.cpp
  ${WIN_DIR}/Registry.cpp
  ${WIN_DIR}/System.cpp
  ${WIN_DIR}/SystemInfo.cpp
  ${WIN_DIR}/TimeUtils.cpp
  
  ${7ZIP_COMMON_DIR}/CreateCoder.cpp
  ${7ZIP_COMMON_DIR}/FilePathAutoRename.cpp
  ${7ZIP_COMMON_DIR}/FileStreams.cpp
  ${7ZIP_COMMON_DIR}/FilterCoder.cpp
  ${7ZIP_COMMON_DIR}/LimitedStreams.cpp
  ${7ZIP_COMMON_DIR}/MethodProps.cpp
  ${7ZIP_COMMON_DIR}/ProgressUtils.cpp
  ${7ZIP_COMMON_DIR}/PropId.cpp
  ${7ZIP_COMMON_DIR}/StreamObjects.cpp
  ${7ZIP_COMMON_DIR}/StreamUtils.cpp
  ${7ZIP_COMMON_DIR}/UniqBlocks.cpp

  ${AR_DIR}/Common/ItemNameUtils.cpp
  ${AR_DIR}/Common/OutStreamWithCRC.cpp

  ${COMPRESS_DIR}/CopyCoder.cpp
  
  ${C_DIR}/Alloc.c
  ${C_DIR}/CpuArch.c
  ${C_DIR}/DllSecur.c
  ${C_DIR}/Sort.c
  ${C_DIR}/Threads.c

  ${UI_DIR}/Console/BenchCon.cpp
  ${UI_DIR}/Console/ConsoleClose.cpp
  ${UI_DIR}/Console/ExtractCallbackConsole.cpp
  ${UI_DIR}/Console/HashCon.cpp
  ${UI_DIR}/Console/List.cpp
  ${UI_DIR}/Console/Main.cpp
  ${UI_DIR}/Console/MainAr.cpp
  ${UI_DIR}/Console/OpenCallbackConsole.cpp
  ${UI_DIR}/Console/PercentPrinter.cpp
  ${UI_DIR}/Console/UpdateCallbackConsole.cpp
  ${UI_DIR}/Console/UserInputUtils.cpp

  ${UI_DIR}/Common/ArchiveCommandLine.cpp
  ${UI_DIR}/Common/ArchiveExtractCallback.cpp
  ${UI_DIR}/Common/ArchiveOpenCallback.cpp
  ${UI_DIR}/Common/Bench.cpp
  ${UI_DIR}/Common/DefaultName.cpp
  ${UI_DIR}/Common/EnumDirItems.cpp
  ${UI_DIR}/Common/Extract.cpp
  ${UI_DIR}/Common/ExtractingFilePath.cpp
  ${UI_DIR}/Common/HashCalc.cpp
  ${UI_DIR}/Common/LoadCodecs.cpp
  ${UI_DIR}/Common/OpenArchive.cpp
  ${UI_DIR}/Common/PropIDUtils.cpp
  ${UI_DIR}/Common/SetProperties.cpp
  ${UI_DIR}/Common/SortUtils.cpp
  ${UI_DIR}/Common/TempFiles.cpp
  ${UI_DIR}/Common/Update.cpp
  ${UI_DIR}/Common/UpdateAction.cpp
  ${UI_DIR}/Common/UpdateCallback.cpp
  ${UI_DIR}/Common/UpdatePair.cpp
  ${UI_DIR}/Common/UpdateProduce.cpp
  
  $<TARGET_OBJECTS:CRC>
)
set_target_properties(Console PROPERTIES OUTPUT_NAME "7z")
target_compile_definitions(Console PRIVATE -DEXTERNAL_CODECS -DWIN_LONG_PATH -D_7ZIP_LARGE_PAGES -DSUPPORT_DEVICE_FILE)

# add_library(Explorer MODULE)
# set_target_properties(Explorer PROPERTIES OUTPUT_NAME "7-zip")

# if(CMAKE_SIZEOF_VOID_P EQUAL 8)
#   add_library(Explorer32 MODULE)
#   set_target_properties(Explorer PROPERTIES OUTPUT_NAME "7-zip32")
# endif()

add_executable(FileManager WIN32
  $<TARGET_OBJECTS:COMMON>
  ${COMMON_DIR}/Lang.cpp
  ${COMMON_DIR}/Random.cpp

  ${WIN_DIR}/Clipboard.cpp
  ${WIN_DIR}/CommonDialog.cpp
  ${WIN_DIR}/DLL.cpp
  ${WIN_DIR}/ErrorMsg.cpp
  ${WIN_DIR}/FileDir.cpp
  ${WIN_DIR}/FileFind.cpp
  ${WIN_DIR}/FileIO.cpp
  ${WIN_DIR}/FileLink.cpp
  ${WIN_DIR}/FileName.cpp
  ${WIN_DIR}/MemoryGlobal.cpp
  ${WIN_DIR}/MemoryLock.cpp
  ${WIN_DIR}/Menu.cpp
  ${WIN_DIR}/ProcessUtils.cpp
  ${WIN_DIR}/PropVariant.cpp
  ${WIN_DIR}/PropVariantConv.cpp
  ${WIN_DIR}/Registry.cpp
  ${WIN_DIR}/ResourceString.cpp
  ${WIN_DIR}/Shell.cpp
  ${WIN_DIR}/Synchronization.cpp
  ${WIN_DIR}/System.cpp
  ${WIN_DIR}/TimeUtils.cpp
  ${WIN_DIR}/Window.cpp

  ${WIN_DIR}/FileSystem.cpp
  ${WIN_DIR}/Net.cpp
  ${WIN_DIR}/SecurityUtils.cpp

  ${WIN_DIR}/Control/ComboBox.cpp
  ${WIN_DIR}/Control/Dialog.cpp
  ${WIN_DIR}/Control/ListView.cpp
  ${WIN_DIR}/Control/PropertyPage.cpp
  ${WIN_DIR}/Control/Window2.cpp

  ${7ZIP_COMMON_DIR}/CreateCoder.cpp
  ${7ZIP_COMMON_DIR}/FilePathAutoRename.cpp
  ${7ZIP_COMMON_DIR}/FileStreams.cpp
  ${7ZIP_COMMON_DIR}/FilterCoder.cpp
  ${7ZIP_COMMON_DIR}/LimitedStreams.cpp
  ${7ZIP_COMMON_DIR}/MethodProps.cpp
  ${7ZIP_COMMON_DIR}/ProgressUtils.cpp
  ${7ZIP_COMMON_DIR}/PropId.cpp
  ${7ZIP_COMMON_DIR}/StreamObjects.cpp
  ${7ZIP_COMMON_DIR}/StreamUtils.cpp
  ${7ZIP_COMMON_DIR}/UniqBlocks.cpp

  ${UI_DIR}/Common/ArchiveExtractCallback.cpp
  ${UI_DIR}/Common/ArchiveName.cpp
  ${UI_DIR}/Common/ArchiveOpenCallback.cpp
  ${UI_DIR}/Common/CompressCall.cpp
  ${UI_DIR}/Common/DefaultName.cpp
  ${UI_DIR}/Common/EnumDirItems.cpp
  ${UI_DIR}/Common/ExtractingFilePath.cpp
  ${UI_DIR}/Common/HashCalc.cpp
  ${UI_DIR}/Common/LoadCodecs.cpp
  ${UI_DIR}/Common/OpenArchive.cpp
  ${UI_DIR}/Common/PropIDUtils.cpp
  ${UI_DIR}/Common/SetProperties.cpp
  ${UI_DIR}/Common/SortUtils.cpp
  ${UI_DIR}/Common/UpdateAction.cpp
  ${UI_DIR}/Common/UpdateCallback.cpp
  ${UI_DIR}/Common/UpdatePair.cpp
  ${UI_DIR}/Common/UpdateProduce.cpp
  ${UI_DIR}/Common/WorkDir.cpp
  ${UI_DIR}/Common/ZipRegistry.cpp

  ${UI_DIR}/Explorer/ContextMenu.cpp
  ${UI_DIR}/Explorer/RegistryContextMenu.cpp

  ${UI_DIR}/GUI/HashGUI.cpp
  ${UI_DIR}/GUI/UpdateCallbackGUI2.cpp

  ${COMPRESS_DIR}/CopyCoder.cpp

  ${C_DIR}/Alloc.c
  ${C_DIR}/Sort.c
  ${C_DIR}/Threads.c

  ${UI_DIR}/FileManager/App.cpp
  ${UI_DIR}/FileManager/BrowseDialog.cpp
  ${UI_DIR}/FileManager/ClassDefs.cpp
  ${UI_DIR}/FileManager/EnumFormatEtc.cpp
  ${UI_DIR}/FileManager/ExtractCallback.cpp
  ${UI_DIR}/FileManager/FileFolderPluginOpen.cpp
  ${UI_DIR}/FileManager/FilePlugins.cpp
  ${UI_DIR}/FileManager/FM.cpp
  ${UI_DIR}/FileManager/FoldersPage.cpp
  ${UI_DIR}/FileManager/FormatUtils.cpp
  ${UI_DIR}/FileManager/FSFolder.cpp
  ${UI_DIR}/FileManager/FSFolderCopy.cpp
  ${UI_DIR}/FileManager/HelpUtils.cpp
  ${UI_DIR}/FileManager/LangUtils.cpp
  ${UI_DIR}/FileManager/MenuPage.cpp
  ${UI_DIR}/FileManager/MyLoadMenu.cpp
  ${UI_DIR}/FileManager/OpenCallback.cpp
  ${UI_DIR}/FileManager/OptionsDialog.cpp
  ${UI_DIR}/FileManager/Panel.cpp
  ${UI_DIR}/FileManager/PanelCopy.cpp
  ${UI_DIR}/FileManager/PanelCrc.cpp
  ${UI_DIR}/FileManager/PanelDrag.cpp
  ${UI_DIR}/FileManager/PanelFolderChange.cpp
  ${UI_DIR}/FileManager/PanelItemOpen.cpp
  ${UI_DIR}/FileManager/PanelItems.cpp
  ${UI_DIR}/FileManager/PanelKey.cpp
  ${UI_DIR}/FileManager/PanelListNotify.cpp
  ${UI_DIR}/FileManager/PanelMenu.cpp
  ${UI_DIR}/FileManager/PanelOperations.cpp
  ${UI_DIR}/FileManager/PanelSelect.cpp
  ${UI_DIR}/FileManager/PanelSort.cpp
  ${UI_DIR}/FileManager/PanelSplitFile.cpp
  ${UI_DIR}/FileManager/ProgramLocation.cpp
  ${UI_DIR}/FileManager/PropertyName.cpp
  ${UI_DIR}/FileManager/RegistryAssociations.cpp
  ${UI_DIR}/FileManager/RegistryPlugins.cpp
  ${UI_DIR}/FileManager/RegistryUtils.cpp
  ${UI_DIR}/FileManager/RootFolder.cpp
  ${UI_DIR}/FileManager/SplitUtils.cpp
  ${UI_DIR}/FileManager/StringUtils.cpp
  ${UI_DIR}/FileManager/SysIconUtils.cpp
  ${UI_DIR}/FileManager/TextPairs.cpp
  ${UI_DIR}/FileManager/UpdateCallback100.cpp
  ${UI_DIR}/FileManager/VerCtrl.cpp
  ${UI_DIR}/FileManager/ViewSettings.cpp
  ${UI_DIR}/FileManager/AboutDialog.cpp
  ${UI_DIR}/FileManager/ComboDialog.cpp
  ${UI_DIR}/FileManager/CopyDialog.cpp
  ${UI_DIR}/FileManager/EditDialog.cpp
  ${UI_DIR}/FileManager/EditPage.cpp
  ${UI_DIR}/FileManager/LangPage.cpp
  ${UI_DIR}/FileManager/ListViewDialog.cpp
  ${UI_DIR}/FileManager/MessagesDialog.cpp
  ${UI_DIR}/FileManager/OverwriteDialog.cpp
  ${UI_DIR}/FileManager/PasswordDialog.cpp
  ${UI_DIR}/FileManager/ProgressDialog2.cpp
  ${UI_DIR}/FileManager/SettingsPage.cpp
  ${UI_DIR}/FileManager/SplitDialog.cpp
  ${UI_DIR}/FileManager/SystemPage.cpp

  ${UI_DIR}/FileManager/AltStreamsFolder.cpp
  ${UI_DIR}/FileManager/FSDrives.cpp
  ${UI_DIR}/FileManager/LinkDialog.cpp
  ${UI_DIR}/FileManager/NetFolder.cpp

  ${UI_DIR}/FileManager/resource.rc

  ${UI_DIR}/Agent/Agent.cpp
  ${UI_DIR}/Agent/AgentOut.cpp
  ${UI_DIR}/Agent/AgentProxy.cpp
  ${UI_DIR}/Agent/ArchiveFolder.cpp
  ${UI_DIR}/Agent/ArchiveFolderOpen.cpp
  ${UI_DIR}/Agent/ArchiveFolderOut.cpp
  ${UI_DIR}/Agent/UpdateCallbackAgent.cpp
)
set_target_properties(FileManager PROPERTIES OUTPUT_NAME "7zFM")
target_compile_definitions(FileManager PRIVATE -DLANG -DNEW_FOLDER_INTERFACE -DEXTERNAL_CODECS -DWIN_LONG_PATH -DSUPPORT_DEVICE_FILE)
target_link_libraries(FileManager comctl32 comdlg32 mpr gdi32 htmlhelp)

add_executable(GUI WIN32
  ${UI_DIR}/GUI/BenchmarkDialog.cpp
  ${UI_DIR}/GUI/CompressDialog.cpp
  ${UI_DIR}/GUI/ExtractDialog.cpp
  ${UI_DIR}/GUI/ExtractGUI.cpp
  ${UI_DIR}/GUI/GUI.cpp
  ${UI_DIR}/GUI/HashGUI.cpp
  ${UI_DIR}/GUI/UpdateCallbackGUI.cpp
  ${UI_DIR}/GUI/UpdateCallbackGUI2.cpp
  ${UI_DIR}/GUI/UpdateGUI.cpp

  $<TARGET_OBJECTS:COMMON>

  ${COMMON_DIR}/CRC.cpp
  ${COMMON_DIR}/CommandLineParser.cpp
  ${COMMON_DIR}/Lang.cpp
  ${COMMON_DIR}/ListFileUtils.cpp
  
  ${WIN_DIR}/Clipboard.cpp
  ${WIN_DIR}/CommonDialog.cpp
  ${WIN_DIR}/DLL.cpp
  ${WIN_DIR}/ErrorMsg.cpp
  ${WIN_DIR}/FileDir.cpp
  ${WIN_DIR}/FileFind.cpp
  ${WIN_DIR}/FileIO.cpp
  ${WIN_DIR}/FileLink.cpp
  ${WIN_DIR}/FileName.cpp
  ${WIN_DIR}/FileSystem.cpp
  ${WIN_DIR}/MemoryGlobal.cpp
  ${WIN_DIR}/MemoryLock.cpp
  ${WIN_DIR}/PropVariant.cpp
  ${WIN_DIR}/PropVariantConv.cpp
  ${WIN_DIR}/Registry.cpp
  ${WIN_DIR}/ResourceString.cpp
  ${WIN_DIR}/Shell.cpp
  ${WIN_DIR}/Synchronization.cpp
  ${WIN_DIR}/System.cpp
  ${WIN_DIR}/TimeUtils.cpp
  ${WIN_DIR}/Window.cpp
  
  ${WIN_DIR}/Control/ComboBox.cpp
  ${WIN_DIR}/Control/Dialog.cpp
  ${WIN_DIR}/Control/ListView.cpp
  
  ${7ZIP_COMMON_DIR}/CreateCoder.cpp
  ${7ZIP_COMMON_DIR}/FilePathAutoRename.cpp
  ${7ZIP_COMMON_DIR}/FileStreams.cpp
  ${7ZIP_COMMON_DIR}/FilterCoder.cpp
  ${7ZIP_COMMON_DIR}/LimitedStreams.cpp
  ${7ZIP_COMMON_DIR}/MethodProps.cpp
  ${7ZIP_COMMON_DIR}/ProgressUtils.cpp
  ${7ZIP_COMMON_DIR}/PropId.cpp
  ${7ZIP_COMMON_DIR}/StreamObjects.cpp
  ${7ZIP_COMMON_DIR}/StreamUtils.cpp
  ${7ZIP_COMMON_DIR}/UniqBlocks.cpp
  
  ${UI_DIR}/Common/ArchiveCommandLine.cpp
  ${UI_DIR}/Common/ArchiveExtractCallback.cpp
  ${UI_DIR}/Common/ArchiveOpenCallback.cpp
  ${UI_DIR}/Common/Bench.cpp
  ${UI_DIR}/Common/DefaultName.cpp
  ${UI_DIR}/Common/EnumDirItems.cpp
  ${UI_DIR}/Common/Extract.cpp
  ${UI_DIR}/Common/ExtractingFilePath.cpp
  ${UI_DIR}/Common/HashCalc.cpp
  ${UI_DIR}/Common/LoadCodecs.cpp
  ${UI_DIR}/Common/OpenArchive.cpp
  ${UI_DIR}/Common/PropIDUtils.cpp
  ${UI_DIR}/Common/SetProperties.cpp
  ${UI_DIR}/Common/SortUtils.cpp
  ${UI_DIR}/Common/TempFiles.cpp
  ${UI_DIR}/Common/Update.cpp
  ${UI_DIR}/Common/UpdateAction.cpp
  ${UI_DIR}/Common/UpdateCallback.cpp
  ${UI_DIR}/Common/UpdatePair.cpp
  ${UI_DIR}/Common/UpdateProduce.cpp
  ${UI_DIR}/Common/WorkDir.cpp
  ${UI_DIR}/Common/ZipRegistry.cpp
  
  ${AR_DIR}/Common/OutStreamWithCRC.cpp
  
  ${UI_DIR}/FileManager/EditDialog.cpp
  ${UI_DIR}/FileManager/ExtractCallback.cpp
  ${UI_DIR}/FileManager/FormatUtils.cpp
  ${UI_DIR}/FileManager/HelpUtils.cpp
  ${UI_DIR}/FileManager/LangUtils.cpp
  ${UI_DIR}/FileManager/ListViewDialog.cpp
  ${UI_DIR}/FileManager/OpenCallback.cpp
  ${UI_DIR}/FileManager/ProgramLocation.cpp
  ${UI_DIR}/FileManager/PropertyName.cpp
  ${UI_DIR}/FileManager/RegistryUtils.cpp
  ${UI_DIR}/FileManager/SplitUtils.cpp
  ${UI_DIR}/FileManager/StringUtils.cpp
  ${UI_DIR}/FileManager/OverwriteDialog.cpp
  ${UI_DIR}/FileManager/PasswordDialog.cpp
  ${UI_DIR}/FileManager/ProgressDialog2.cpp

  ${UI_DIR}/FileManager/BrowseDialog.cpp
  ${UI_DIR}/FileManager/ComboDialog.cpp
  ${UI_DIR}/FileManager/SysIconUtils.cpp
  
  ${UI_DIR}/Explorer/MyMessages.cpp
  
  ${COMPRESS_DIR}/CopyCoder.cpp
  
  ${C_DIR}/Alloc.c
  ${C_DIR}/CpuArch.c
  ${C_DIR}/Sort.c
  ${C_DIR}/Threads.c
  
  $<TARGET_OBJECTS:CRC>
  
  ${UI_DIR}/GUI/resource.rc
)
set_target_properties(GUI PROPERTIES OUTPUT_NAME "7zG")
target_compile_definitions(GUI PRIVATE -DLANG -DEXTERNAL_CODECS -DWIN_LONG_PATH -DSUPPORT_DEVICE_FILE -D_7ZIP_LARGE_PAGES)
target_link_libraries(GUI comctl32 comdlg32 gdi32 htmlhelp)

# add_executable(BundleAlone )
# set_target_properties(BundleAlone PROPERTIES OUTPUT_NAME "7za")

# add_executable(BundleAlone7z )
# set_target_properties(BundleAlone7z PROPERTIES OUTPUT_NAME "7zr")

# add_executable(BundleFM WIN32)
# set_target_properties(BundleFM PROPERTIES OUTPUT_NAME "7zFM")

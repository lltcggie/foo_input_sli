if(MSVC)
	if(USE_FFMPEG AND NOT FFMPEG_LIBRARIES)
		add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/avcodec.lib ${CMAKE_CURRENT_BINARY_DIR}/avcodec.exp
			COMMAND lib
			ARGS /def:${CMAKE_CURRENT_SOURCE_DIR}/avcodec-vgmstream-59.def /machine:x86 /out:${CMAKE_CURRENT_BINARY_DIR}/avcodec.lib
			DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/avcodec-vgmstream-59.def)
		add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/avformat.lib ${CMAKE_CURRENT_BINARY_DIR}/avformat.exp
			COMMAND lib
			ARGS /def:${CMAKE_CURRENT_SOURCE_DIR}/avformat-vgmstream-59.def /machine:x86 /out:${CMAKE_CURRENT_BINARY_DIR}/avformat.lib
			DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/avformat-vgmstream-59.def)
		add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/avutil.lib ${CMAKE_CURRENT_BINARY_DIR}/avutil.exp
			COMMAND lib
			ARGS /def:${CMAKE_CURRENT_SOURCE_DIR}/avutil-vgmstream-57.def /machine:x86 /out:${CMAKE_CURRENT_BINARY_DIR}/avutil.lib
			DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/avutil-vgmstream-57.def)
		add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/swresample.lib ${CMAKE_CURRENT_BINARY_DIR}/swresample.exp
			COMMAND lib
			ARGS /def:${CMAKE_CURRENT_SOURCE_DIR}/swresample-vgmstream-4.def /machine:x86 /out:${CMAKE_CURRENT_BINARY_DIR}/swresample.lib
			DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/swresample-vgmstream-4.def)
		file(GLOB AVCODEC_HEADERS "${VGM_SOURCE_DIR}/ext_includes/ffmpeg/libavcodec/*.h")
		file(GLOB AVFORMAT_HEADERS "${VGM_SOURCE_DIR}/ext_includes/ffmpeg/libavformat/*.h")
		file(GLOB AVUTIL_HEADERS "${VGM_SOURCE_DIR}/ext_includes/ffmpeg/libavutil/*.h")
		file(GLOB SWRESAMPLE_HEADERS "${VGM_SOURCE_DIR}/ext_includes/ffmpeg/libswresample/*.h")
		add_custom_target(ffmpeg
			DEPENDS
				${CMAKE_CURRENT_BINARY_DIR}/avcodec.lib
				${CMAKE_CURRENT_BINARY_DIR}/avformat.lib
				${CMAKE_CURRENT_BINARY_DIR}/avutil.lib
				${CMAKE_CURRENT_BINARY_DIR}/swresample.lib
			SOURCES
				${AVCODEC_HEADERS}
				${AVFORMAT_HEADERS}
				${AVUTIL_HEADERS}
				${SWRESAMPLE_HEADERS})
		source_group("Header Files\\ffmpeg\\libavcodec" FILES ${AVCODEC_HEADERS})
		source_group("Header Files\\ffmpeg\\libavformat" FILES ${AVFORMAT_HEADERS})
		source_group("Header Files\\ffmpeg\\libavutil" FILES ${AVUTIL_HEADERS})
		source_group("Header Files\\ffmpeg\\libswresample" FILES ${SWRESAMPLE_HEADERS})
	endif()
elseif(MINGW)
	if(NOT DLLTOOL)
		set(DLLTOOL dlltool)
	endif()

	if(USE_FFMPEG AND NOT FFMPEG_LIBRARIES)
		add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/avcodec.lib
			COMMAND ${DLLTOOL}
			ARGS -d ${CMAKE_CURRENT_SOURCE_DIR}/avcodec-vgmstream-59.def -l ${CMAKE_CURRENT_BINARY_DIR}/avcodec.lib -D avcodec-vgmstream-59.dll
			DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/avcodec-vgmstream-59.def)
		add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/avformat.lib
			COMMAND ${DLLTOOL}
			ARGS -d ${CMAKE_CURRENT_SOURCE_DIR}/avformat-vgmstream-59.def -l ${CMAKE_CURRENT_BINARY_DIR}/avformat.lib -D avformat-vgmstream-59.dll
			DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/avformat-vgmstream-59.def)
		add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/avutil.lib
			COMMAND ${DLLTOOL}
			ARGS -d ${CMAKE_CURRENT_SOURCE_DIR}/avutil-vgmstream-57.def -l ${CMAKE_CURRENT_BINARY_DIR}/avutil.lib -D avutil-vgmstream-57.dll
			DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/avutil-vgmstream-57.def)
		add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/swresample.lib
			COMMAND ${DLLTOOL}
			ARGS -d ${CMAKE_CURRENT_SOURCE_DIR}/swresample-vgmstream-4.def -l ${CMAKE_CURRENT_BINARY_DIR}/swresample.lib -D swresample-vgmstream-4.dll
			DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/swresample-vgmstream-4.def)
		add_custom_target(ffmpeg
			DEPENDS
				${CMAKE_CURRENT_BINARY_DIR}/avcodec.lib
				${CMAKE_CURRENT_BINARY_DIR}/avformat.lib
				${CMAKE_CURRENT_BINARY_DIR}/avutil.lib
				${CMAKE_CURRENT_BINARY_DIR}/swresample.lib)
	endif()
endif()

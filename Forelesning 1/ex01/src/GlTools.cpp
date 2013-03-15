#include "GlTools.h"

#include <cstdlib>
#include <sstream>
#include <vector>
#include <assert.h>
#include <iostream>

namespace GlTools {

GLuint compileShader(std::string& src, GLenum type) {
	// create shader object
	GLuint s = glCreateShader(type);
	if (s == 0) {
		std::stringstream ss;
		ss << "Failed to create shader of type " << type << std::endl;
		THROW_EXCEPTION(ss.str());
	}

	// set source code and compile
	const GLchar* src_list[1] = { src.c_str() };
	glShaderSource(s, 1, src_list, NULL);
	glCompileShader(s);

	// check for errors
	GLint compile_status;
	glGetShaderiv(s, GL_COMPILE_STATUS, &compile_status);
	if (compile_status != GL_TRUE) {
		// compilation failed
		std::stringstream ss;
		ss << "Compilation failed!" << std::endl;
		ss << "--- source code ---" << std::endl;
		ss << src << std::endl;

		GLint logsize;
		glGetShaderiv(s, GL_INFO_LOG_LENGTH, &logsize);

		if (logsize > 0) {
			std::vector < GLchar > infolog(logsize + 1);
			glGetShaderInfoLog(s, logsize, NULL, &infolog[0]);

			ss << "--- error log ---" << std::endl;
			ss << std::string(infolog.begin(), infolog.end()) << std::endl;
		} else {
			ss << "--- empty log message ---" << std::endl;
		}
		ss << "Exiting." << std::endl;
		THROW_EXCEPTION(ss.str());
	}
	return s;
}


void linkProgram(GLuint program) {
	// link
	glLinkProgram(program);

	// check for errors
	GLint linkstatus;
	glGetProgramiv(program, GL_LINK_STATUS, &linkstatus);
	if (linkstatus != GL_TRUE) {
		std::stringstream ss;
		ss << "Linking failed!" << std::endl;

		GLint logsize;
		glGetProgramiv(program, GL_INFO_LOG_LENGTH, &logsize);

		if (logsize > 0) {
			std::vector < GLchar > infolog(logsize + 1);
			glGetProgramInfoLog(program, logsize, NULL, &infolog[0]);
			ss << "--- error log ---" << std::endl;
			ss << std::string(infolog.begin(), infolog.end()) << std::endl;
		} else {
			ss << "--- empty log message ---" << std::endl;
		}
		ss << "Exiting." << std::endl;
		THROW_EXCEPTION(ss.str());
	}
}

} // namespace GlTools

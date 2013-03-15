#ifndef _GLTOOLS_H_
#define _GLTOOLS_H_
#include <sstream>
#include <string>
#include <GL/glew.h>

#include "GameException.h"

/**
 * Simple assert that checks for OpenGL errors, and throws
 * a runtime error if anything goes wrong
 */
#define ASSERT_GL do { \
    GLenum ASSERT_GL_err = glGetError(); \
    if( ASSERT_GL_err != GL_NO_ERROR ) { \
		std::stringstream ASSERT_GL_string; \
		ASSERT_GL_string << __FILE__ << '@' << __LINE__ << ": OpenGL error:" \
             << std::hex << ASSERT_GL_err << " " << gluErrorString(ASSERT_GL_err); \
			 THROW_EXCEPTION(ASSERT_GL_string.str()); \
    } \
} while(0);

namespace GlTools {
/**
 * Compiles a shader string into a shader object
 * @param src Shader source code
 * @param type Fragment, geometry or vertex shader?
 * @return name of the compiled shader
 */
GLuint compileShader(std::string& src, GLenum type);

/**
 * Links a shader program
 * @param program The name of the program to link
 */
void linkProgram(GLuint program);
} // namespace GlTools

#endif // _GLTOOLS_H_

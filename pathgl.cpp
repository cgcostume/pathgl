
#include <GL/glew.h>
#include <GL/freeglut.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <iostream>
#include <fstream>
#include <sstream>
#include <cstring>
#include <vector>
#include <hash_map>
#include <random>
#include <algorithm>
#include <iterator>

GLuint rect(-1);

GLuint tracevert(-1);
GLuint tracefrag(-1);
GLuint traceprog(-1);

GLuint framebuffer(-1);
GLuint texture(-1);

int frame(-1);

glm::vec3 eye   (278.f, 273.f,-800.f);
glm::vec3 center(eye + glm::vec3(0.f, 0.f, 1.f));
glm::vec3 up(0.f, 1.f, 0.f);

float fovy(140.0);

GLint viewport[2] = { 520, 520 };

glm::mat4 transform;

GLuint verticesImage(-1);
GLuint indicesImage(-1);
GLuint colorsImage(-1);
GLuint hsphereImage(-1);

GLuint u_frame(-1);
GLuint u_accum(-1);
GLuint u_eye(-1);
GLuint u_transform(-1);
GLuint u_viewport(-1);


const bool error(
    const char * file
,   const int line)
{
    const GLenum error(glGetError());
    const bool errorOccured(GL_NO_ERROR != error);

    if(errorOccured) 
        printf("OpenGL: %s [0x%x %s : %i]\n", glewGetErrorString(error), error, file, line);

    return errorOccured;
}

#define glError() error(__FILE__, __LINE__)


void shader_error(const GLuint shader)
{
    GLint status(GL_FALSE);
    glGetShaderiv(shader, GL_COMPILE_STATUS, &status);

    if(GL_TRUE == status)
        return;

    GLint maxLength(0);
    GLint logLength(0);

	glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &maxLength);
    GLchar *log = new GLchar[maxLength];

    glGetShaderInfoLog(shader, maxLength, &logLength, log);

    if(logLength)
        std::cerr << log;
}

void update_source(const GLuint shader, const char * filepath)
{
	std::ifstream stream(filepath, std::ios::in);
	if(!stream)
	{
        std::cerr << "Read from \"" << filepath << "\" failed." << std::endl;
        return;
    }

	std::ostringstream source;
    source << stream.rdbuf();
    stream.close();

    const std::string str(source.str());
    const GLchar * chr(str.c_str());

    glShaderSource(shader, 1, &chr, nullptr);
    glCompileShader(shader); shader_error(shader);
}

void clear()
{
    frame = -1;

    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    glClear(GL_COLOR_BUFFER_BIT);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void update()
{
    glError();

    update_source(tracevert, "trace.vert");
    update_source(tracefrag, "trace.frag");

    glLinkProgram(traceprog);
    glUseProgram(traceprog);
    glError();

    u_transform = glGetUniformLocation(traceprog, "transform");
    u_frame     = glGetUniformLocation(traceprog, "frame");
    u_accum     = glGetUniformLocation(traceprog, "accum");
    u_eye       = glGetUniformLocation(traceprog, "eye");
    u_viewport  = glGetUniformLocation(traceprog, "viewport");

    const glm::vec2 viewportf(viewport[0], viewport[1]);

    if(u_transform != -1)
        glUniformMatrix4fv(u_transform, 1, GL_FALSE, glm::value_ptr(transform));
    if(u_eye != -1)
        glUniform3fv(u_eye, 1, glm::value_ptr(eye));
    if(u_viewport != -1)
        glUniform4f(u_viewport, viewportf.x, viewportf.y, 1.f / viewportf.x, 1.f / viewportf.y);

	GLuint u_vertices = glGetUniformLocation(traceprog, "vertices");
	GLuint u_indices  = glGetUniformLocation(traceprog, "indices");
	GLuint u_colors   = glGetUniformLocation(traceprog, "colors");
    GLuint u_source   = glGetUniformLocation(traceprog, "source");
    GLuint u_hsphere  = glGetUniformLocation(traceprog, "hsphere");

	if(u_source != -1)
		glUniform1i(u_source,   0);
	if(u_vertices != -1)
		glUniform1i(u_vertices, 1);
	if(u_indices != -1)
		glUniform1i(u_indices,  2);
	if(u_colors != -1)
		glUniform1i(u_colors,   3);
    if(u_hsphere != -1)
        glUniform1i(u_hsphere,  4);

    glError();

    clear();
}

void on_reshape(int w, int h)
{
    glError();

    viewport[0] = w;
    viewport[1] = h;

    const glm::vec2 viewportf(viewport[0], viewport[1]);

    glViewport(0, 0, w, h);
    glError();

    const glm::mat4 perspective(glm::perspective(fovy, viewportf.y / viewportf.x, 1.f, 2.f));
    const glm::mat4 view(glm::lookAt(eye, center, up));
    const glm::mat4 model(1);

    transform = perspective * view * model;

    glUniformMatrix4fv(u_transform, 1, GL_FALSE, glm::value_ptr(transform));
    glUniform4f(u_viewport, viewportf.x, viewportf.y, 1.f / viewportf.x, 1.f / viewportf.y);

    // resize fbo textures

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, viewport[0], viewport[1], 0, GL_RGBA, GL_FLOAT, 0);
    //glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, w, h, 0, GL_RGBA, GL_FLOAT, 0);
	glError();

    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);  
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);  
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);  
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST); 

    glError();

    clear();
}

void on_display()
{
    glUniform1i(u_frame, ++frame);
    glUniform1f(u_accum, static_cast<float>(frame) / static_cast<float>(frame + 1));

    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, framebuffer);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

    glBindFramebuffer(GL_READ_FRAMEBUFFER, framebuffer);
    glBlitFramebuffer(0, 0, viewport[0], viewport[1], 0, 0, viewport[0], viewport[1], GL_COLOR_BUFFER_BIT, GL_NEAREST);

    glFlush();
}

void on_keyboard(unsigned char key,	int x, int y)
{
    switch(key)
    {
    case 27: // ESC key
		exit(0);
        break;

    default:
		break;
    }
    glutPostRedisplay();
}

void on_special(int key, int x,	int y)
{
    switch(key)
    {
	case GLUT_KEY_F5:
        update();
		break;

	default:
		break;
    }
    glutPostRedisplay();
}

void on_timer(int value)
{
    glutTimerFunc(0, on_timer, 0);
    glutPostRedisplay();
}

const glm::uint splitEdge(
    const glm::uint a
,   const glm::uint b
,   std::vector<glm::vec3> & points
,   std::hash_map<glm::highp_uint, glm::uint> & cache)
{
    const bool aSmaller(a < b);
    const glm::highp_uint smaller(aSmaller ? a : b);
    const glm::highp_uint greater(aSmaller ? b : a);
    const glm::highp_uint hash((smaller << 32) + greater);

    std::hash_map<glm::highp_uint, glm::uint>::const_iterator h(cache.find(hash));
    if(cache.end() != h)
        return h->second;

    const glm::vec3 & a3(points[a]);
    const glm::vec3 & b3(points[b]);

    const glm::vec3 s(glm::normalize((a3 + b3) * 0.5f));

    points.push_back(s);
    const glm::uint i(static_cast<glm::uint>(points.size()) - 1);

    cache[hash] = i;

    return i;
}


void pointsOnSphere(
    std::vector<glm::vec3> & points
,   const unsigned int minN)
{
    // approach to create evenly distributed points on a sphere
    // 1. create points of icosphere
    // 2. cutout hemisphere
    // 3. randomize point list

    // 1. create an icosphere

    static const float t = (1.f + sqrtf(5.f)) * 0.5f;

    std::vector<glm::vec3> icopoints;
    std::vector<glm::uvec3> icofaces;

    // basic icosahedron

    icopoints.push_back(glm::vec3(-1, t, 0));
    icopoints.push_back(glm::vec3( 1, t, 0));
    icopoints.push_back(glm::vec3(-1,-t, 0));
    icopoints.push_back(glm::vec3( 1,-t, 0));

    icopoints.push_back(glm::vec3( 0,-1, t));
    icopoints.push_back(glm::vec3( 0, 1, t));
    icopoints.push_back(glm::vec3( 0,-1,-t));
    icopoints.push_back(glm::vec3( 0, 1,-t));

    icopoints.push_back(glm::vec3( t, 0,-1));
    icopoints.push_back(glm::vec3( t, 0, 1));
    icopoints.push_back(glm::vec3(-t, 0,-1));
    icopoints.push_back(glm::vec3(-t, 0, 1));

    // normalize
    for(int i = 0; i < 12; ++i)
        icopoints[i] = glm::normalize(icopoints[i]);

    icofaces.push_back(glm::uvec3(  0, 11,  5));
    icofaces.push_back(glm::uvec3(  0,  5,  1));
    icofaces.push_back(glm::uvec3(  0,  1,  7));
    icofaces.push_back(glm::uvec3(  0,  7, 10));
    icofaces.push_back(glm::uvec3(  0, 10, 11));

    icofaces.push_back(glm::uvec3(  1,  5,  9));
    icofaces.push_back(glm::uvec3(  5, 11,  4));
    icofaces.push_back(glm::uvec3( 11, 10,  2));
    icofaces.push_back(glm::uvec3( 10,  7,  6));
    icofaces.push_back(glm::uvec3(  7,  1,  8));

    icofaces.push_back(glm::uvec3(  3,  9,  4));
    icofaces.push_back(glm::uvec3(  3,  4,  2));
    icofaces.push_back(glm::uvec3(  3,  2,  6));
    icofaces.push_back(glm::uvec3(  3,  6,  8));
    icofaces.push_back(glm::uvec3(  3,  8,  9));

    icofaces.push_back(glm::uvec3(  4,  9,  5));
    icofaces.push_back(glm::uvec3(  2,  4, 11));
    icofaces.push_back(glm::uvec3(  6,  2, 10));
    icofaces.push_back(glm::uvec3(  8,  6,  7));
    icofaces.push_back(glm::uvec3(  9,  8,  1));

    // iterative triangle refinement - split each triangle 
    // into 4 new ones and create points appropriately.

    const int r = static_cast<int>(log(static_cast<float>(minN * 2 / 12.f)) / log(4.f) + .5f); // N = 12 * 4 ^ r

    std::hash_map<glm::highp_uint, glm::uint> cache;

    for(int i = 0; i < r; ++i)
    {
        const int size(static_cast<int>(icofaces.size()));

        for(int f = 0; f < size; ++f)
        {
            glm::uvec3 & face(icofaces[f]);

            const glm::uint  a(face.x);
            const glm::uint  b(face.y);
            const glm::uint  c(face.z);

            const glm::uint ab(splitEdge(a, b, icopoints, cache));
            const glm::uint bc(splitEdge(b, c, icopoints, cache));
            const glm::uint ca(splitEdge(c, a, icopoints, cache));

            face = glm::uvec3(ab, bc, ca);

            icofaces.push_back(glm::uvec3(a, ab, ca));
            icofaces.push_back(glm::uvec3(b, bc, ab));
            icofaces.push_back(glm::uvec3(c, ca, bc));
        }
    }

    // 2. remove lower hemisphere

    const int size(static_cast<int>(icopoints.size()));
    for(int i = 0; i < size; ++i)
        if(icopoints[i].y > 0.f)
            points.push_back(icopoints[i]);

    // 3. shuffle all points of hemisphere

    std::random_device rd;
    std::mt19937 engine(rd()); 

    std::shuffle(points.begin(), points.end(), engine);
}


int main(int argc, char** argv)
{
    // GLUT & GLEW

	glutInit(&argc, argv);
    
    glutInitContextVersion(3, 1);
    //glutInitContextProfile(GLUT_COMPATIBILITY_PROFILE);
    //glutInitContextFlags(GLUT_FORWARD_COMPATIBLE);

    glutInitDisplayMode(GLUT_RGBA | GLUT_SINGLE);
    glutInitWindowSize(viewport[0], viewport[1]);

    glutCreateWindow("Minimal GLSL Path Tracer v1 - Daniel Limberger");
    glewInit();

    glutDisplayFunc (on_display);
    glutReshapeFunc (on_reshape);
    glutKeyboardFunc(on_keyboard);
    glutSpecialFunc (on_special);

    glutTimerFunc(0, on_timer, 0);

    // RECT

    static const GLfloat vs[] = {+1.f,-1.f,+1.f,+1.f,-1.f,-1.f,-1.f,+1.f};
    glGenBuffers(1, &rect);
    glBindBuffer(GL_ARRAY_BUFFER, rect);
    glBufferData(GL_ARRAY_BUFFER, 8 * sizeof(GLfloat), vs, GL_STATIC_DRAW);
    glError();

    // CREATE FBO

    glActiveTexture(GL_TEXTURE0);
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, viewport[0], viewport[1], 0, GL_RGBA, GL_FLOAT, 0);
    glError();
    
    glGenFramebuffers(1, &framebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture, 0);
    glError();

    const GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    glError();
    if(GL_FRAMEBUFFER_COMPLETE != status)
        std::cerr << "Frame Buffer Object incomplete." << std::endl;

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glError();

    // SHADER
    
    tracevert = glCreateShader(GL_VERTEX_SHADER);
    tracefrag = glCreateShader(GL_FRAGMENT_SHADER);
    traceprog = glCreateProgram();
    glError();

    glAttachShader(traceprog, tracevert);
    glAttachShader(traceprog, tracefrag);
    glError();

    update();

    // CONFIG

    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);

    const int a_vertex = glGetAttribLocation(traceprog, "a_vertex");

    glBindBuffer(GL_ARRAY_BUFFER, rect);
    glVertexAttribPointerARB(a_vertex, 2, GL_FLOAT, 0, 0, 0);
    glEnableVertexAttribArrayARB(a_vertex);
    glError();

	// CREATE GEOMETRY

	// http://www.graphics.cornell.edu/online/box/data.html


	std::vector<glm::vec3> vertices;
	// lights
	vertices.push_back(glm::vec3( 343.0, 548.7998, 227.0)); // 00
	vertices.push_back(glm::vec3( 343.0, 548.7998, 332.0)); // 01
	vertices.push_back(glm::vec3( 213.0, 548.7998, 332.0)); // 02
	vertices.push_back(glm::vec3( 213.0, 548.7998, 227.0)); // 03
	// room
	vertices.push_back(glm::vec3(   0.0,   0.0,   0.0)); // 04 
	vertices.push_back(glm::vec3(   0.0,   0.0, 559.2)); // 05 
	vertices.push_back(glm::vec3(   0.0, 548.8,   0.0)); // 06
	vertices.push_back(glm::vec3(   0.0, 548.8, 559.2)); // 07
	vertices.push_back(glm::vec3( 552.8,   0.0,   0.0)); // 08 
	vertices.push_back(glm::vec3( 549.6,   0.0, 559.2)); // 09
	vertices.push_back(glm::vec3( 556.0, 548.8,   0.0)); // 10
	vertices.push_back(glm::vec3( 556.0, 548.8, 559.2)); // 11
	// short block
	vertices.push_back(glm::vec3( 290.0,   0.0, 114.0)); // 12
	vertices.push_back(glm::vec3( 290.0, 165.0, 114.0)); // 13
	vertices.push_back(glm::vec3( 240.0,   0.0, 272.0)); // 14
	vertices.push_back(glm::vec3( 240.0, 165.0, 272.0)); // 15
	vertices.push_back(glm::vec3(  82.0,   0.0, 225.0)); // 16
	vertices.push_back(glm::vec3(  82.0, 165.0, 225.0)); // 17
	vertices.push_back(glm::vec3( 130.0,   0.0,  65.0)); // 18
	vertices.push_back(glm::vec3( 130.0, 165.0,  65.0)); // 19
	// tall block
	vertices.push_back(glm::vec3( 423.0,   0.0, 247.0)); // 20
	vertices.push_back(glm::vec3( 423.0, 330.0, 247.0)); // 21
	vertices.push_back(glm::vec3( 472.0,   0.0, 406.0)); // 22
	vertices.push_back(glm::vec3( 472.0, 330.0, 406.0)); // 23
	vertices.push_back(glm::vec3( 314.0,   0.0, 456.0)); // 24
	vertices.push_back(glm::vec3( 314.0, 330.0, 456.0)); // 25
	vertices.push_back(glm::vec3( 265.0,   0.0, 296.0)); // 26
	vertices.push_back(glm::vec3( 265.0, 330.0, 296.0)); // 27

	std::vector<glm::uvec4> indices;
	// light
	indices.push_back(glm::uvec4(  0,  1,  2, 0));
	indices.push_back(glm::uvec4(  0,  2,  3, 0));
	// room floor
	indices.push_back(glm::uvec4(  8,  4,  5, 1));
	indices.push_back(glm::uvec4(  8,  5,  9, 1));
	// room ceiling
	indices.push_back(glm::uvec4( 10, 11,  7, 1));
	indices.push_back(glm::uvec4( 10,  7,  6, 1));
	// room back wall
	indices.push_back(glm::uvec4(  9,  5,  7, 1));
	indices.push_back(glm::uvec4(  9,  7, 11, 1));
	// room right wall
	indices.push_back(glm::uvec4(  5,  4,  6, 3));
	indices.push_back(glm::uvec4(  5,  6,  7, 3));
	// room left wall
	indices.push_back(glm::uvec4(  8,  9, 11, 2));
	indices.push_back(glm::uvec4(  8, 11, 10, 2));
	// short block
	indices.push_back(glm::uvec4( 19, 17, 15, 1));
	indices.push_back(glm::uvec4( 19, 15, 13, 1));
	indices.push_back(glm::uvec4( 12, 13, 15, 1));
	indices.push_back(glm::uvec4( 12, 15, 14, 1));
	indices.push_back(glm::uvec4( 18, 19, 13, 1));
	indices.push_back(glm::uvec4( 18, 13, 12, 1));
	indices.push_back(glm::uvec4( 16, 17, 19, 1));
	indices.push_back(glm::uvec4( 16, 19, 18, 1));
	indices.push_back(glm::uvec4( 14, 15, 17, 1));
	indices.push_back(glm::uvec4( 14, 17, 16, 1));
	// tall block
	indices.push_back(glm::uvec4( 27, 25, 23, 1));
	indices.push_back(glm::uvec4( 27, 23, 21, 1));
	indices.push_back(glm::uvec4( 20, 21, 23, 1));
	indices.push_back(glm::uvec4( 20, 23, 22, 1));
	indices.push_back(glm::uvec4( 26, 27, 21, 1));
	indices.push_back(glm::uvec4( 26, 21, 20, 1));
	indices.push_back(glm::uvec4( 24, 25, 27, 1));
	indices.push_back(glm::uvec4( 24, 27, 26, 1));
	indices.push_back(glm::uvec4( 22, 23, 25, 1));
	indices.push_back(glm::uvec4( 22, 25, 24, 1));

	std::vector<glm::vec4> colors;
	colors.push_back(glm::vec4(0.0, 0.0, 0.0, 1.0)); // 0 black
	colors.push_back(glm::vec4(1.0, 1.0, 1.0, 1.0)); // 1 white
	colors.push_back(glm::vec4(1.0, 0.0, 0.0, 1.0)); // 2 red
	colors.push_back(glm::vec4(0.0, 1.0, 0.0, 1.0)); // 3 green

	// CREATE TEXTURES

	glActiveTexture(GL_TEXTURE1);

	glGenTextures(1, &verticesImage);
	glBindTexture(GL_TEXTURE_1D, verticesImage);
	glTexImage1D(GL_TEXTURE_1D, 0, GL_RGB32F, static_cast<GLsizei>(vertices.size())
		, 0, GL_RGB, GL_FLOAT, &vertices[0]);
	glError();
	glTexParameterf(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);  
	glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);  
	glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_NEAREST); 

	glActiveTexture(GL_TEXTURE2);

	glGenTextures(1, &indicesImage);
	glBindTexture(GL_TEXTURE_1D, indicesImage);
	glTexImage1D(GL_TEXTURE_1D, 0, GL_RGBA8UI, static_cast<GLsizei>(indices.size())
		, 0, GL_RGBA_INTEGER, GL_UNSIGNED_INT, &indices[0]);
	glError();
	glTexParameterf(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);  
	glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);  
	glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_NEAREST); 

	glActiveTexture(GL_TEXTURE3);

	glGenTextures(1, &colorsImage);
	glBindTexture(GL_TEXTURE_1D, colorsImage);
	glTexImage1D(GL_TEXTURE_1D, 0, GL_RGBA32F, static_cast<GLsizei>(colors.size())
		, 0, GL_RGBA, GL_FLOAT, &colors[0]);
	glError();
	glTexParameterf(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);  
	glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);  
	glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

    // CREATE HEMISPHERE SAMPLES

    std::vector<glm::vec3> points;
    pointsOnSphere(points, static_cast<unsigned int>(1e5));

    const GLsizei samplerSize = static_cast<GLsizei>(sqrt(points.size()));
    while(points.size() > samplerSize * samplerSize)
        points.pop_back();

	glActiveTexture(GL_TEXTURE4);

	glGenTextures(1, &hsphereImage);
	glBindTexture(GL_TEXTURE_2D, hsphereImage);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, samplerSize, samplerSize
		, 0, GL_RGB, GL_FLOAT, &points[0]);
	glError();
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);  
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);  
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);  
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

    glActiveTexture(GL_TEXTURE0);

    // START

    glutMainLoop();

    return 0;
}
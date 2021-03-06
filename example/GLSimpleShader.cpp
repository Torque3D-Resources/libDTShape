/*
Copyright (C) 2013 James S Urquhart

Permission is hereby granted, free of charge, to any person
obtaining a copy of this software and associated documentation
files (the "Software"), to deal in the Software without
restriction, including without limitation the rights to use,
copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the
Software is furnished to do so, subject to the following
conditions:

The above copyright notice and this permission notice shall be
included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.
*/

#include "GLSimpleShader.h"
#include "core/log.h"

#ifdef HAVE_OPENGLES2
const char* GLSimpleShader::sStandardVertexProgram = "\n\
\n\
attribute vec4 aPosition;\n\
attribute vec4 aColor;\n\
attribute vec2 aTexCoord0;\n\
attribute vec3 aNormal;\n\
\n\
uniform mat4 worldMatrixProjection;\n\
uniform mat4 worldMatrix;\n\
uniform vec3 lightPos;\n\
uniform vec3 lightColor;\n\
\n\
varying vec2 vTexCoord0;\n\
varying vec4 vColor0;\n\
\n\
void main()\n\
{\n\
vec3 normal, lightDir;\n\
vec4 diffuse;\n\
float NdotL;\n\
\n\
normal = normalize(mat3(worldMatrix) * aNormal);\n\
\n\
lightDir = normalize(vec3(lightPos));\n\
\n\
NdotL = max(dot(normal, lightDir), 0.0);\n\
\n\
diffuse = vec4(lightColor, 1.0);\n\
\n\
gl_Position = worldMatrixProjection * aPosition;\n\
vTexCoord0 = aTexCoord0;\n\
vColor0 = NdotL * diffuse;\n\
vColor0.a = 1.0;\n\
}\n\
";

const char* GLSimpleShader::sSkinnedVertexProgram = "\n\
\n\
attribute vec4 aPosition;\n\
attribute vec4 aColor;\n\
attribute vec2 aTexCoord0;\n\
attribute vec3 aNormal;\n\
\n\
uniform mat4 worldMatrixProjection;\n\
uniform mat4 worldMatrix;\n\
uniform vec3 lightPos;\n\
uniform vec3 lightColor;\n\
\n\
varying vec2 vTexCoord0;\n\
varying vec4 vColor0;\n\
\n\
void main()\n\
{\n\
vec3 normal, lightDir;\n\
vec4 diffuse;\n\
float NdotL;\n\
\n\
normal = normalize(mat3(worldMatrix) * aNormal);\n\
\n\
lightDir = normalize(vec3(lightPos));\n\
\n\
NdotL = max(dot(normal, lightDir), 0.0);\n\
\n\
diffuse = vec4(lightColor, 1.0);\n\
\n\
gl_Position = worldMatrixProjection * aPosition;\n\
vTexCoord0 = aTexCoord0;\n\
vColor0 = NdotL * diffuse;\n\
vColor0.a = 1.0;\n\
}\n\
";

const char* GLSimpleShader::sStandardFragmentProgram = "\n\
\n\
varying vec2 vTexCoord0;\n\
varying vec4 vColor0;\n\
uniform sampler2D texture0;\n\
\n\
void main()\n\
{\n\
gl_FragColor = texture2D(texture0, vTexCoord0);\n\
gl_FragColor.r = gl_FragColor.r * vColor0.r * vColor0.a;\n\
gl_FragColor.g = gl_FragColor.g * vColor0.g * vColor0.a;\n\
gl_FragColor.b = gl_FragColor.b * vColor0.b * vColor0.a;\n\
}\n\
";
#else
const char* GLSimpleShader::sStandardVertexProgram = "\n\
#version 120\n\
\n\
attribute vec4 aPosition;\n\
attribute vec4 aColor;\n\
attribute vec2 aTexCoord0;\n\
attribute vec3 aNormal;\n\
\n\
uniform mat4 worldMatrixProjection;\n\
uniform mat4 worldMatrix;\n\
uniform vec3 lightPos;\n\
uniform vec3 lightColor;\n\
\n\
varying vec2 vTexCoord0;\n\
varying vec4 vColor0;\n\
\n\
void main()\n\
{\n\
vec3 normal, lightDir;\n\
vec4 diffuse;\n\
float NdotL;\n\
\n\
normal = normalize(mat3(worldMatrix) * aNormal);\n\
\n\
lightDir = normalize(vec3(lightPos));\n\
\n\
NdotL = max(dot(normal, lightDir), 0.0);\n\
\n\
diffuse = vec4(lightColor, 1.0);\n\
\n\
gl_Position = worldMatrixProjection * aPosition;\n\
vTexCoord0 = aTexCoord0;\n\
vColor0 = NdotL * diffuse;\n\
vColor0.a = 1.0;\n\
}\n\
";

const char* GLSimpleShader::sSkinnedVertexProgram = "\n\
#version 120\n\
\n\
attribute vec3 aPosition;\n\
attribute vec4 aColor;\n\
attribute vec2 aTexCoord0;\n\
attribute vec3 aNormal;\n\
attribute vec4 aBlendIndices;\n\
attribute vec4 aBlendWeights;\n\
\n\
uniform mat4 worldMatrixProjection;\n\
uniform mat4 worldMatrix;\n\
uniform vec3 lightPos;\n\
uniform vec3 lightColor;\n\
\n\
uniform mat4 boneTransforms[64];\n\
\n\
varying vec2 vTexCoord0;\n\
varying vec4 vColor0;\n\
\n\
void main()\n\
{\n\
vec3 normal, lightDir;\n\
vec4 diffuse;\n\
float NdotL;\n\
\n\
vec3 inPosition;\n\
vec3 inNormal;\n\
vec3 posePos = vec3(0.0);\n\
vec3 poseNormal = vec3(0.0);\n\
for (int i=0; i<4; i++) {\n\
  mat4 mat = boneTransforms[int(aBlendIndices[i])];\n\
  mat3 m33 = mat3x3(mat);\n\
  posePos += (mat * vec4(aPosition, 1)).xyz * aBlendWeights[i];\n\
  poseNormal += (m33 * aNormal) * aBlendWeights[i];\n\
}\n\
inPosition = posePos;\n\
inNormal = normalize(poseNormal);\n\
\n\
normal = normalize(mat3(worldMatrix) * inNormal);\n\
\n\
lightDir = normalize(vec3(lightPos));\n\
\n\
NdotL = max(dot(normal, lightDir), 0.0);\n\
\n\
diffuse = vec4(lightColor, 1.0);\n\
\n\
gl_Position = worldMatrixProjection * vec4(inPosition, 1);\n\
vTexCoord0 = aTexCoord0;\n\
vColor0 = NdotL * diffuse;\n\
vColor0.a = 1.0;\n\
}\n\
";

const char* GLSimpleShader::sStandardFragmentProgram = "\n\
#version 120\n\
\n\
varying vec2 vTexCoord0;\n\
varying vec4 vColor0;\n\
uniform sampler2D texture0;\n\
\n\
void main()\n\
{\n\
gl_FragColor = texture2D(texture0, vTexCoord0);\n\
gl_FragColor.r = gl_FragColor.r * vColor0.r * vColor0.a;\n\
gl_FragColor.g = gl_FragColor.g * vColor0.g * vColor0.a;\n\
gl_FragColor.b = gl_FragColor.b * vColor0.b * vColor0.a;\n\
}\n\
";
#endif

GLSimpleShader::GLSimpleShader(const char *vert, const char *fragment) :
mProjectionMatrix(1),
mModelViewMatrix(1)
{
  GLuint program[2];
  
  program[0] = compile(GL_VERTEX_SHADER, vert);
  program[1] = compile(GL_FRAGMENT_SHADER, fragment);
  
  mProgram = linkProgram(program);
  
  glDeleteShader(program[0]);
  glDeleteShader(program[1]);
  
  dMemset(mLightPos, '\0', sizeof(mLightPos));
  dMemset(mLightColor, '\0', sizeof(mLightColor));
}

GLSimpleShader::~GLSimpleShader()
{
  if (mProgram != 0)
     glDeleteProgram(mProgram);
}

GLuint GLSimpleShader::linkProgram(GLuint *shaders)
{
  GLuint program = glCreateProgram();
  
  glAttachShader(program, shaders[0]);
  glAttachShader(program, shaders[1]);
  
  glBindAttribLocation(program, kGLSimpleVertexAttrib_Position, "aPosition");
  glBindAttribLocation(program, kGLSimpleVertexAttrib_Color,  "aColor");
  glBindAttribLocation(program, kGLSimpleVertexAttrib_TexCoords, "aTexCoord0");
  glBindAttribLocation(program, kGLSimpleVertexAttrib_Normal, "aNormal");
  glBindAttribLocation(program, kGLSimpleVertexAttrib_BlendIndices, "aBlendIndices");
  glBindAttribLocation(program, kGLSimpleVertexAttrib_BlendWeights, "aBlendWeights");

  glLinkProgram(program);
  
  GLint status = GL_TRUE;
  glGetProgramiv (program, GL_LINK_STATUS, &status);
  if (status == GL_FALSE)
  {
     GLint infoLogLength = 0;
     glGetProgramiv(program, GL_INFO_LOG_LENGTH, &infoLogLength);
	 
	 if (infoLogLength > 0)
	 {
	     GLchar *strInfoLog = new GLchar[infoLogLength + 1];
	     glGetProgramInfoLog(program, infoLogLength, NULL, strInfoLog);
     
	     Log::printf("Failed to link shader...\n%s\n", strInfoLog);
     
	     delete[] strInfoLog;
	 }
     glDeleteProgram(program);
     return 0;
  }
  
  glDetachShader(program, shaders[0]);
  glDetachShader(program, shaders[1]);
  
  glUseProgram(program);
  
  mUniforms[0] = glGetUniformLocation(program, "worldMatrixProjection");
  mUniforms[1] = glGetUniformLocation(program, "worldMatrix");
  mUniforms[2] = glGetUniformLocation(program, "lightPos");
  mUniforms[3] = glGetUniformLocation(program, "lightColor");
  mUniforms[4] = glGetUniformLocation(program, "sampler");
  mUniforms[5] = glGetUniformLocation(program, "boneTransforms");
  
  return program;
}

GLuint GLSimpleShader::compile(GLenum shaderType, const char *data)
{
  GLuint shader = glCreateShader(shaderType);
  glShaderSource(shader, 1, &data, NULL);
  glCompileShader(shader);
  
  GLint status;
  glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
  if (status == GL_FALSE)
  {
     GLint infoLogLength;
     glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLogLength);
     
     GLchar *strInfoLog = new GLchar[infoLogLength + 1];
     glGetShaderInfoLog(shader, infoLogLength, NULL, strInfoLog);
     
     Log::errorf("Failed to compile shader(%i)...\n%s", shaderType, strInfoLog);
     
     delete[] strInfoLog;
     return 0;
  }
  
  return shader;
}

void GLSimpleShader::use()
{
  glUseProgram(mProgram);
}

void GLSimpleShader::setLightPosition(Point3F pos)
{
  mLightPos[0] = pos.x;
  mLightPos[1] = pos.y;
  mLightPos[2] = pos.z;
}

void GLSimpleShader::setLightColor(ColorF col)
{
  mLightColor[0] = col.red;
  mLightColor[1] = col.green;
  mLightColor[2] = col.blue;
}

void GLSimpleShader::setProjectionMatrix(MatrixF &proj)
{
  mProjectionMatrix = proj;
}

void GLSimpleShader::setModelViewMatrix(MatrixF &modelview)
{
  mModelViewMatrix = modelview;
}

void GLSimpleShader::updateBoneTransforms(U32 numTransforms, MatrixF *transformList)
{
  glUniformMatrix4fv(mUniforms[kGLSimpleUniformBoneTransforms], numTransforms, GL_TRUE, (F32*)transformList);
}

void GLSimpleShader::updateTransforms()
{
  MatrixF combined = mProjectionMatrix * mModelViewMatrix;
  combined.transpose();
  glUniformMatrix4fv(mUniforms[kGLSimpleUniformMVPMatrix], 1, GL_FALSE, (F32*)combined);
  combined = mModelViewMatrix;
  combined.transpose();
  glUniformMatrix4fv(mUniforms[kGLSimpleUniformMVMatrix], 1, GL_FALSE, (F32*)combined);
  
  glUniform3fv(mUniforms[kGLSimpleUniformLightColor], 1, mLightColor);
  glUniform3fv(mUniforms[kGLSimpleUniformLightPos], 1, mLightPos);
}


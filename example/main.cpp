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

#include "platform/platform.h"

#define GL_GLEXT_PROTOTYPES

#include "SDL.h"
#include "GLIncludes.h"
#include <stdio.h>

#include "libdtshape.h"

#include "ts/tsShape.h"
#include "ts/tsShapeInstance.h"
#include "ts/tsRenderState.h"
#include "ts/tsMaterialManager.h"
#include "core/color.h"
#include "math/mMath.h"
#include "math/util/frustum.h"
#include "core/stream/fileStream.h"

extern "C"
{
#include "soil/SOIL.h"
}


#include "main.h"
#include "GLSimpleShader.h"
#include "GLMaterial.h"
#include "GLTSMeshRenderer.h"

#define TICK_TIME 32

const char* GetAssetPath(const char *file);

// DTS Sequences for sample shape

const char* sTSAppSequenceNames[] = {
   "Root",
   "Run",
   "Crouch_Backward",
   "Side",
   "Jump",
   "invalid"
};

AnimSequenceInfo sAppSequenceInfos[] = {
   {50, 109, true},
   {150, 169, true},
   {460, 489, true},
   {200, 219, true},
   {1000, 1010, false}
};

GLStateTracker gGLStateTracker;

AppState *AppState::sInstance = NULL;

TSMaterialManager *TSMaterialManager::instance()
{
   return AppState::getInstance()->sMaterialManager;
}

AppState::AppState()
{
   running = false;
   window = NULL;
   dStrcpy(shapeToLoad, "cube.dae");
   sInstance = this;
   
   sShape = NULL;
   sShapeInstance = NULL;
   
   sThread = NULL;
   sCurrentSequenceIdx = 0;
   for (int i=0; i<kNumTSAppSequences; i++)
      sSequences[i] = -1;
   
   sShader = NULL;
   mCurrentShader = NULL;
      
   sRot = 180.0f;
   sDeltaRot = 0.0f;
   sCameraPosition = Point3F(0,-10,1);
   sModelPosition = Point3F(0,0,0);
   deltaCameraPos = Point3F(0,0,0);
   
   sOldTicks = 0;
   sRenderState = NULL;
   
   sMaterialManager = new GLTSMaterialManager();
}

AppState::~AppState()
{
   if (sShader)
      delete sShader;
   if (sRenderState)
      delete sRenderState;
   if (sMaterialManager)
      delete sMaterialManager;
   CleanupShape();
   
   if (window)
   {
      SDL_GL_DeleteContext(glcontext);
      SDL_DestroyWindow(window);
   }
   sInstance = NULL;
}

void AppState::onKeyChanged(int key, int state)
{
   switch (key)
   {
      case SDLK_ESCAPE:
         SDL_Quit();
         break;
      case SDLK_w:
         deltaCameraPos.y = state == 1 ? 1.0 : 0.0;
         break;
      case SDLK_a:
         deltaCameraPos.x = state == 1 ? -1.0 : 0.0;
         break;
      case SDLK_s:
         deltaCameraPos.y = state == 1 ? -1.0 : 0.0;
         break;
      case SDLK_d:
         deltaCameraPos.x = state == 1 ? 1.0 : 0.0;
         break;
      case SDLK_q:
         sDeltaRot = state == 1 ? -30 : 0.0;
         break;
      case SDLK_e:
         sDeltaRot = state == 1 ? 30 : 0.0;
         break;
         
      case SDLK_k:
         // Rotate shape left
         if (sShape && state == 0)
         {
            if (sCurrentSequenceIdx == 0)
               sCurrentSequenceIdx = kNumTSAppSequences-1;
            else
               sCurrentSequenceIdx--;
            if (sSequences[sCurrentSequenceIdx] != -1)
               SwitchToSequence(sSequences[sCurrentSequenceIdx], 0.5f, true);
         }
         break;
      case SDLK_l:
         // Rotate shape right
         if (sShape && state == 0)
         {
            if (sCurrentSequenceIdx == kNumTSAppSequences-1)
               sCurrentSequenceIdx = 0;
            else
               sCurrentSequenceIdx++;
            if (sSequences[sCurrentSequenceIdx] != -1)
               SwitchToSequence(sSequences[sCurrentSequenceIdx], 0.5f, true);
         }
         break;
   }
}

void AppState::mainLoop()
{
   // Process events
   U32 tickTime = SDL_GetTicks();
   U32 deltaTick = (tickTime - sOldTicks);
   if (deltaTick > 1000)
      deltaTick = 1000;
   F32 deltaTime = deltaTick / 1000.0f;
   sOldTicks = tickTime;
   
   SDL_Event e;
   
   // Check input
   while (SDL_PollEvent(&e))
   {
      switch (e.type)
      {
         case SDL_QUIT:
            running = false;
            return;
            break;
         case SDL_KEYDOWN:
            onKeyChanged(e.key.keysym.sym, 1);
            break;
         case SDL_KEYUP:
            onKeyChanged(e.key.keysym.sym, 0);
            break;
      }
   }
   
   // Draw scene
   glClearColor(1,1,1,1); // Use OpenGL commands, see the OpenGL reference.
   glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // clearing screen
   
   SetupProjection();
   DrawShape(deltaTime);
   
   SDL_GL_SwapWindow(window);
   
   // Slow down if we're going too fast
   U32 tickEnd = SDL_GetTicks();
   deltaTick = (tickEnd - tickTime);
   if (deltaTick < TICK_TIME)
      SDL_Delay(TICK_TIME - deltaTick);
}

bool AppState::LoadShape()
{
   TSMaterialManager::instance()->allocateAndRegister("Soldier_Dif");
   TSMaterialManager::instance()->allocateAndRegister("Soldier_Dazzle");
   TSMaterialManager::instance()->mapMaterial("base_Soldier_Main", "Soldier_Dif");
   TSMaterialManager::instance()->mapMaterial("base_Soldier_Dazzle", "Soldier_Dazzle");
   
   
   TSMaterial *cubeMat = TSMaterialManager::instance()->allocateAndRegister("cube");
   TSMaterialManager::instance()->mapMaterial("Cube", "cube");
   
   const char* fullPath = GetAssetPath(shapeToLoad);
   sShape = TSShape::createFromPath(fullPath);
   
   if (!sShape)
   {
      return false;
   }
   
   sShapeInstance = new TSShapeInstance(sShape, sRenderState, true);
   
   // Load all dsq files
   for (int i=0; i<kNumTSAppSequences; i++)
   {
      //FileStream dsqFile;
      char pathName[64];
      dSprintf(pathName, 64, "player_%s.dts", sTSAppSequenceNames[i]);
      /*if (dsqFile.open(GetAssetPath(pathName), FileStream::Read) && sShape->importSequences(&dsqFile, ""))
      {
         Log::printf("Sequence file %s loaded", pathName);
      }*/
      if (sShape->addSequence(GetAssetPath(pathName), "", sTSAppSequenceNames[i], sAppSequenceInfos[i].start, sAppSequenceInfos[i].end, true, false))
      {
         Log::printf("Sequence file %s loaded", pathName);
      }
   }
   
   // Resolve all sequences
   for (int i=0; i<kNumTSAppSequences; i++)
   {
      sSequences[i] = sShape->findSequence(sTSAppSequenceNames[i]);
      if (sSequences[i] != -1)
      {
         if (sAppSequenceInfos[i].cyclic)
            sShape->sequences[sSequences[i]].flags |= TSShape::Cyclic;
         else
            sShape->sequences[sSequences[i]].flags &= ~TSShape::Cyclic;
      }
   }
   
   sThread = sShapeInstance->addThread();
   if (sSequences[kTSRootAnim] != -1)
   {
      sShapeInstance->setSequence(sThread, sSequences[kTSRootAnim], 0);
      sShapeInstance->setTimeScale(sThread, 1.0f);
   }
   
   // Load DTS
   sShape->initRender();
   return true;
}

void AppState::CleanupShape()
{
   if (sShapeInstance)
   {
      delete sShapeInstance;
   }
   
   if (sShape)
   {
      delete sShape;
   }
}

void AppState::DrawShape(F32 dt)
{
   GLTSSceneRenderState dummyScene;
   
   sCameraPosition += deltaCameraPos * dt;
   sRot += sDeltaRot * dt;
   
   glEnable(GL_DEPTH_TEST);
   
   MatrixF calc(1);
   
   // Set render transform
   dummyScene.mProjectionMatrix = sProjectionMatrix;
   dummyScene.mViewMatrix = MatrixF(1);
   dummyScene.mWorldMatrix = MatrixF(1);
   dummyScene.mWorldMatrix.setPosition(sModelPosition);
   
   // Apply model rotation
   calc = MatrixF(1);
   AngAxisF rot2 = AngAxisF(Point3F(0,0,1), mDegToRad(sRot));
   rot2.setMatrix(&calc);
   dummyScene.mWorldMatrix *= calc;
   
   // Set camera position
   calc = MatrixF(1);
   calc.setPosition(sCameraPosition);
   dummyScene.mViewMatrix *= calc;
   
   // Rotate camera
   rot2 = AngAxisF(Point3F(1,0,0), mDegToRad(-90.0f));
   calc = MatrixF(1);
   rot2.setMatrix(&calc);
   dummyScene.mViewMatrix *= calc;
   
   // Calculate lighting vector
   mLightPos = -sCameraPosition;
   dummyScene.mViewMatrix.mulV(mLightPos);
   
   // Calculate base modelview
   MatrixF invView = dummyScene.mViewMatrix;
   invView.inverse();
   MatrixF glModelView = dummyScene.mWorldMatrix;
   glModelView *= invView;
   
   // Set base shader uniforms
   mCurrentShader = sShader;
   mCurrentShader->setProjectionMatrix(dummyScene.mProjectionMatrix);
   mCurrentShader->setModelViewMatrix(glModelView);
   mCurrentShader->setLightPosition(mLightPos);
   mCurrentShader->setLightColor(ColorF(1,1,1,1));
   mCurrentShader->use();
   
   // Now we can render
   sRenderState->setSceneState(&dummyScene);
   sRenderState->reset();
   
   // Animate & render shape to the renderState
   sShapeInstance->beginUpdate(sRenderState);
   sShapeInstance->advanceTime(dt);
   sShapeInstance->setCurrentDetail(0);
   sShapeInstance->animateNodeSubtrees(true);
   sShapeInstance->animate();
   sShapeInstance->render(*sRenderState);
   
   // Ensure generated primitives are in the right z-order
   sRenderState->sortRenderInsts();
   
   // Render solid stuff
   for (int i=0; i<sRenderState->mRenderInsts.size(); i++)
   {
      sRenderState->mRenderInsts[i]->render(sRenderState);
   }
  
   // Render translucent stuff
   for (int i=0; i<sRenderState->mTranslucentRenderInsts.size(); i++)
   {
      // Update transform, & render buffers
      sRenderState->mTranslucentRenderInsts[i]->render(sRenderState);
   }
   
   glDisable(GL_DEPTH_TEST);
}

void AppState::SwitchToSequence(U32 seqIdx, F32 transitionTime, bool doTransition)
{
   Log::printf("Changing to sequence %s", sTSAppSequenceNames[seqIdx]);
   if (doTransition)
   {
      // Smoothly transition to new sequence
      if (sSequences[seqIdx] != -1)
      {
         F32 pos = 0.0f;
         if (sThread->getSeqIndex() == sSequences[seqIdx])
         {
            // Keep existing position if same sequence
            pos = sShapeInstance->getPos(sThread);
         }
         sShapeInstance->transitionToSequence(sThread,sSequences[seqIdx],
                                              pos, transitionTime, true);
      }
   }
   else
   {
      // Reset to new sequence
      if (sSequences[seqIdx] != -1)
      {
         sShapeInstance->setSequence(sThread, sSequences[seqIdx], 0.0f);
      }
   }
}

void AppState::SetupProjection()
{
   F32 viewportWidth = 800;
   F32 viewportHeight = 600;
   F32 viewportFOV = 70.0f;
   F32 aspectRatio = viewportWidth / viewportHeight;
   
   F32 viewportNear = 0.1f;
   F32 viewportFar = 200.0f;
   
   F32 wheight = viewportNear * mTan(viewportFOV / 2.0f);
   F32 wwidth = aspectRatio * wheight;
   
   F32 hscale = wwidth * 2.0f / viewportWidth;
   F32 vscale = wheight * 2.0f / viewportHeight;
   
   F32 left = (0) * hscale - wwidth;
   F32 right = (viewportWidth) * hscale - wwidth;
   F32 top = wheight - vscale * (0);
   F32 bottom = wheight - vscale * (viewportHeight);
   
   // Set & grab frustum
   Frustum frustum;
   frustum.set( false, left, right, top, bottom, viewportNear, viewportFar );
   frustum.getProjectionMatrix(&sProjectionMatrix, false);
}

int AppState::main(int argc, char **argv)
{
   DTShapeInit::init();
   
   Log::addConsumer(OnAppLog);
   
   if (SDL_Init(SDL_INIT_EVERYTHING) < 0) {
      Log::errorf("Couldn't initialize SDL: %s\n", SDL_GetError());
      return (1);
   }
   
   int screenW = 800;
   int screenH = 600;
   SDL_DisplayMode mode;
   
   int windowFlags = 0;
   
#ifdef HAVE_OPENGLES2
   windowFlags = SDL_WINDOW_OPENGL|SDL_WINDOW_RESIZABLE|SDL_WINDOW_BORDERLESS;
#else
   windowFlags = SDL_WINDOW_OPENGL|SDL_WINDOW_RESIZABLE;
#endif
   
   SDL_GetCurrentDisplayMode(0, &mode);
   if (mode.w < screenW)
	   screenW = mode.w;
   if (mode.h < screenH)
	   screenH = mode.h;
   
   window = SDL_CreateWindow("libDTShape Example", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, screenW, screenH, windowFlags);
   
   SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
   SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 16);
#ifdef HAVE_OPENGLES2
   SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
   SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
   SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
#else
   SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
   SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
#endif
   
   // Create an OpenGL context associated with the window.
   glcontext = SDL_GL_CreateContext(window);
   SDL_GL_MakeCurrent(window, glcontext);
   
   SDL_GL_GetDrawableSize(window, &screenW, &screenH);
   glViewport(0, 0, screenW, screenH);
   
#ifndef HAVE_OPENGLES2
   GLenum err = glewInit();
   if (GLEW_OK != err)
   {
      Log::errorf("Error: %s\n", glewGetErrorString(err));
      return false;
   }
   
   Log::errorf("Using GLEW %s", glewGetString(GLEW_VERSION));
#endif
   
   sShader = new GLSimpleShader(GLSimpleShader::sStandardVertexProgram, GLSimpleShader::sStandardFragmentProgram);
   sSkinningShader = new GLSimpleShader(GLSimpleShader::sSkinnedVertexProgram, GLSimpleShader::sStandardFragmentProgram);
   mCurrentShader = NULL;

   // Init render state
   sRenderState = new TSRenderState();
   
   bool loaded = LoadShape();
   
   if (!loaded)
   {
      Log::errorf("Couldn't load shape %s", shapeToLoad);
      return false;
   }
   
   running = true;
   return true;
}

void AppState::OnAppLog(U32 level, LogEntry *logEntry)
{
   switch (logEntry->mLevel)
   {
      case LogEntry::Normal:
         fprintf(stdout, "%s\n", logEntry->mData);
         break;
      case LogEntry::Warning:
         fprintf(stdout, "%s\n", logEntry->mData);
         break;
      case LogEntry::Error:
         fprintf(stderr, "%s\n", logEntry->mData);
         break;
      default:
         break;
   }
}

AppState *AppState::getInstance()
{
   return sInstance;
}

int main(int argc, char *argv[])
{
   AppState *app = new AppState();
   if (app->main(argc, argv))
   {
      while (app->running)
         app->mainLoop();
   }
   
   delete app;
   SDL_Quit();
   
   return (0);
}

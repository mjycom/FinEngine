//
// Created by iFinVer on 2016/11/27.
//
#include "finengine.h"
#include "utils.h"
#include "log.h"
#include "ShaderHolder.h"
#include <GLES2/gl2ext.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <getopt.h>
#include <math.h>
#include <dlfcn.h>

FinEngineHolder *pHolder;

JNIEXPORT int JNICALL
Java_com_ifinver_finengine_FinEngine__1startEngine(JNIEnv *env, jclass type, int frameWidth, int frameHeight, jobject jAssetsManager) {
//    void *dso = dlopen("/system/lib/egl/libEGL_mali.so", RTLD_LAZY);
//    if (dso != 0) {
//        LOGI("dlopen: SUCCEEDED");
//    }
//    else {
//        LOGI("dlopen: FAILED! Loading functions in common way!");
//    }
    pHolder = newOffScreenGLContext(env, frameWidth, frameHeight, jAssetsManager);

    if (pHolder == NULL) {
        return -1;
    }
    return pHolder->inputTexture;
}

void JNICALL Java_com_ifinver_finengine_FinEngine__1stopEngine(JNIEnv *env, jclass type) {
    releaseEngine();
}

void JNICALL Java_com_ifinver_finengine_FinEngine_process(JNIEnv *env, jclass, jobject surfaceTexture, jint mFrameDegree, jboolean mirror,
                                                          jbyteArray _data) {
    jboolean isCopy;
    jbyte *data = env->GetByteArrayElements(_data, &isCopy);
    if (isCopy) {
        LOGE("传递数组时发生了Copy！");
    }
//    env->CallVoidMethod(surfaceTexture, pHolder->midAttachToGlContext, pHolder->inputTexture);
    env->CallVoidMethod(surfaceTexture, pHolder->midUpdateTexImage);
    renderFrame(mFrameDegree, mirror, data);
//    env->CallVoidMethod(surfaceTexture, pHolder->midDetachFromGLContext);
    env->ReleaseByteArrayElements(_data, data, 0);
}

void renderFrame(jint degree, jboolean mirror, jbyte *buff) {
    //输入顶点
    glEnableVertexAttribArray(pHolder->localVertexPos);
    glVertexAttribPointer(pHolder->localVertexPos, 2, GL_FLOAT, GL_FALSE, 0, VERTICES_COORD);

    //输入纹理坐标，处理旋转和镜像
    glEnableVertexAttribArray(pHolder->localTexturePos);
    {
        degree %= 360;
        if (degree < 0) degree += 360;
        int idx;
        if (mirror) {
            degree = 360 - degree;
            idx = degree / 90 * 2;
            glVertexAttribPointer(pHolder->localTexturePos, 2, GL_FLOAT, GL_FALSE, 0, TEXTURE_COORD_MIRROR + idx);
        } else {
            idx = degree / 90 * 2;
            glVertexAttribPointer(pHolder->localTexturePos, 2, GL_FLOAT, GL_FALSE, 0, TEXTURE_COORD_NOR + idx);
        }
    }

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_EXTERNAL_OES, pHolder->inputTexture);
    glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glUniform1i(pHolder->localInputTexture, 0);

    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

    glDisableVertexAttribArray(pHolder->localVertexPos);
    glDisableVertexAttribArray(pHolder->localTexturePos);
    //done.
    glReadPixels(0, 0, pHolder->frameWidth, pHolder->frameHeight, GL_RGBA, GL_UNSIGNED_BYTE, buff);
}

FinEngineHolder *newOffScreenGLContext(JNIEnv *env, int frameWidth, int frameHeight, jobject jAssetsManager) {
    /**
     * 初始化Context
     */
    EGLDisplay display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (display == EGL_NO_DISPLAY) {
        checkGlError("eglGetDisplay");
        return NULL;
    }
    EGLint majorVer, minVer;
    if (!eglInitialize(display, &majorVer, &minVer)) {
        checkGlError("eglInitialize");
        return NULL;
    }
    const EGLint confAttr[] =
            {
                    EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,// very important!
                    EGL_SURFACE_TYPE, EGL_PBUFFER_BIT,//EGL_WINDOW_BIT EGL_PBUFFER_BIT we will create a pixelbuffer surface
                    EGL_RED_SIZE, 8,
                    EGL_GREEN_SIZE, 8,
                    EGL_BLUE_SIZE, 8,
                    EGL_ALPHA_SIZE, 8,// if you need the alpha channel
                    EGL_DEPTH_SIZE, 0,// if you need the depth buffer
                    EGL_STENCIL_SIZE, 0,
                    EGL_NONE
            };
    EGLConfig config;
    EGLint numConfigs;
    if (!eglChooseConfig(display, confAttr, &config, 1, &numConfigs)) {
        checkGlError("eglChooseConfig");
        return NULL;
    }
    EGLint attrib_list[] =
            {
                    EGL_CONTEXT_CLIENT_VERSION, 2,
                    EGL_NONE
            };
    EGLContext eglContext = eglCreateContext(display, config, EGL_NO_CONTEXT, attrib_list);
    if (eglContext == EGL_NO_CONTEXT) {
        checkGlError("eglCreateContext");
        return NULL;
    }
    const EGLint surfaceAttr[] = {
            EGL_WIDTH, frameWidth,
            EGL_HEIGHT, frameHeight,
            EGL_NONE
    };
    EGLSurface eglSurface = eglCreatePbufferSurface(display, config, surfaceAttr);
    if (!eglMakeCurrent(display, eglSurface, eglSurface, eglContext)) {
        checkGlError("eglMakeCurrent");
        return NULL;
    }



    /**
     * 初始化FrameBuffer
     */
    FinEngineHolder *holder = new FinEngineHolder();
    glGenFramebuffers(1, &holder->objFrameBuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, holder->objFrameBuffer);

    /**
     * 以RenderBuffer作为依附
     */
//    glGenRenderbuffers(1, &holder->objRenderBuffer);
//    glBindRenderbuffer(GL_RENDERBUFFER, holder->objRenderBuffer);
//    glRenderbufferStorage(GL_RENDERBUFFER, GL_RGBA8_OES, frameWidth, frameHeight);
//    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, holder->objRenderBuffer);

    /**
     * 以Texture作为依附
     */
    glGenTextures(1, &holder->objRenderBuffer);
    glBindTexture(GL_TEXTURE_2D, holder->objRenderBuffer);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, frameWidth, frameHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                           GL_TEXTURE_2D, holder->objRenderBuffer, 0);

    holder->frameWidth = frameWidth;
    holder->frameHeight = frameHeight;

    glDepthMask(GL_FALSE);
    glDisable(GL_BLEND);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_STENCIL_TEST);
    glDisable(GL_DITHER);

    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    bool success = false;
    switch (status) {
        case GL_FRAMEBUFFER_COMPLETE:
            success = true;
            break;
        case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
            LOGE("引擎启动失败！msg = [%s]", "GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT");
            break;
        case GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS:
            LOGE("引擎启动失败！msg = [%s]", "GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS");
            break;
        case GL_FRAMEBUFFER_UNSUPPORTED:
            LOGE("引擎启动失败！msg = [%s]", "GL_FRAMEBUFFER_UNSUPPORTED");
            break;
        default:
            LOGE("引擎启动失败！msg = [%s],code=[%d]", "un know", status);
            break;
    }

    if (success) {
        //创建时默认为无滤镜
        ShaderHolder shaderHolder(FILTER_TYPE_NORMAL, env, jAssetsManager);
        GLuint program = createProgram(shaderHolder.getVertexShader(), shaderHolder.getFragmentShader());
        if (program == 0) {
            return NULL;
        }
        glUseProgram(program);
        holder->program = program;
        holder->localVertexPos = (GLuint) glGetAttribLocation(program, "aPosition");
        holder->localTexturePos = (GLuint) glGetAttribLocation(program, "aTexCoord");
        holder->localInputTexture = (GLuint) glGetUniformLocation(program, "sTexture");

        //硬件缓存 concern not use it.
//        GLuint vertexBuff;
//        glGenBuffers(1, &vertexBuff);
//        glBindBuffer(GL_ARRAY_BUFFER, vertexBuff);
//        glBufferData(GL_ARRAY_BUFFER, sizeof(VERTICES_COORD), VERTICES_COORD, GL_STATIC_DRAW);
//        holder->vertexBuff = vertexBuff;

        //tex
        glGenTextures(1, &holder->inputTexture);

        //输入纹理的方法
        jclass jcSurfaceTexture = env->FindClass("android/graphics/SurfaceTexture");
//        holder->midAttachToGlContext = env->GetMethodID(jcSurfaceTexture, "attachToGLContext", "(I)V");
//        holder->midDetachFromGLContext = env->GetMethodID(jcSurfaceTexture, "detachFromGLContext", "()V");
        holder->midUpdateTexImage = env->GetMethodID(jcSurfaceTexture, "updateTexImage", "()V");
    } else {
        return NULL;
    }

    return holder;
}

void releaseEngine() {
    if (pHolder != NULL) {
        glDeleteTextures(1, &pHolder->inputTexture);
        glDeleteProgram(pHolder->program);
        glDeleteRenderbuffers(1, &pHolder->objRenderBuffer);
        glDeleteFramebuffers(1, &pHolder->objFrameBuffer);
//        delete[](pHolder->positions);
        delete (pHolder);
    }
}

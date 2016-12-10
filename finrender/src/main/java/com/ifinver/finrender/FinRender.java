package com.ifinver.finrender;

import android.graphics.SurfaceTexture;
import android.os.Handler;
import android.os.HandlerThread;
import android.os.Looper;
import android.os.Message;
import android.os.Process;
import android.util.Log;
import android.view.Surface;

/**
 * Created by iFinVer on 2016/12/6.
 * ilzq@foxmail.com
 */

public class FinRender {
    static {
        System.loadLibrary("fin-render-lib");
    }

    private static final String TAG = "FinRender";

    private RenderThread mRenderThread;
    private boolean isPrepared = false;
    private FinRenderListener mListener;
    private int mSurfaceWidth;
    private int mSurfaceHeight;


    private FinRender(Surface output, int width, int height, FinRenderListener listener) {
        mRenderThread = new RenderThread();
        mRenderThread.start();

        this.mListener = listener;
        this.mSurfaceWidth = width;
        this.mSurfaceHeight = height;
        mRenderThread.prepare(output);
    }

    public static FinRender prepare(Surface output, int width, int height, FinRenderListener listener) {
        return new FinRender(output,width,height,listener);
    }

    public void release() {
        this.mListener = null;
        mRenderThread.release();
    }

    public int getInputTex() {
        return mRenderThread.inputTex;
    }

    public long getSharedCtx() {
        return mRenderThread.eglContext;
    }

    public Object getLocker() {
        return mRenderThread.mLocker;
    }

    public void onSizeChange(int width, int height) {
        this.mSurfaceWidth = width;
        this.mSurfaceHeight = height;
        mRenderThread.resize();
    }

    private class RenderThread extends HandlerThread implements Handler.Callback, SurfaceTexture.OnFrameAvailableListener {
        private final int MSG_INIT = 0x101;
        private final int MSG_RELEASE = 0x104;
        private final int MSG_PROCESS = 0x105;

        private Handler mSelfHandler;
        private Handler mMainHandler;
        private boolean delayStart = false;
        private Surface mOutputSurface;
        private SurfaceTexture mInputSurface;
        private long mRenderEngine;
        private final Object mLocker;
        private int inputTex;
        private long eglContext;

        public RenderThread() {
            super("FinRecorderThread", Process.THREAD_PRIORITY_URGENT_DISPLAY);
            mLocker = new Object();
            mMainHandler = new Handler(Looper.getMainLooper());
        }

        @Override
        protected void onLooperPrepared() {
            synchronized (RenderThread.class) {
                mSelfHandler = new Handler(getLooper(), this);
                if (delayStart) {
                    delayStart = false;
                    mSelfHandler.sendEmptyMessage(MSG_INIT);
                }
            }
        }

        @Override
        public boolean handleMessage(Message msg) {
            switch (msg.what) {
                case MSG_INIT:
                    init();
                    return true;
                case MSG_RELEASE:
                    releaseInternal();
                    return true;
                case MSG_PROCESS:
                    process();
                    return true;
            }
            return false;
        }

        private void releaseInternal() {
            stopOutputInternal();
            nativeRelease(mRenderEngine);
            mRenderEngine = 0;
            Log.d(TAG, "渲染引擎已释放");
            quitSafely();
        }

        private void init() {
            Log.d(TAG, "渲染引擎开始初始化");
            mRenderEngine = nativeCreate(mOutputSurface);
            isPrepared = mRenderEngine != 0;
            if (isPrepared) {
                Log.d(TAG, "渲染引擎初始化完成");
            } else {
                Log.e(TAG, "渲染引擎初始化出错");
            }
            inputTex = nativeGetInputTex(mRenderEngine);
            eglContext = nativeGetEglContext(mRenderEngine);
            mInputSurface = new SurfaceTexture(inputTex);
            mInputSurface.setOnFrameAvailableListener(this);
            mInputSurface.setDefaultBufferSize(mSurfaceWidth, mSurfaceHeight);
            if (mListener != null) {
                mMainHandler.post(new Runnable() {
                    @Override
                    public void run() {
                        if(mListener != null) {
                            mListener.onRenderPrepared(isPrepared, mInputSurface, inputTex, eglContext, mSurfaceWidth, mSurfaceHeight);
                        }
                    }
                });
            }
        }

        private void process() {
            if (isPrepared) {
                synchronized (mLocker) {
                    nativeRenderOut(mRenderEngine, mInputSurface);
                }
                if (mListener != null) {
                    mListener.onFrameRendered();
                }
            }
        }

        private void stopOutputInternal() {
            mInputSurface.setOnFrameAvailableListener(null);
            mInputSurface = null;
        }

        @Override
        public void onFrameAvailable(SurfaceTexture surfaceTexture) {
            if (isPrepared) {
                mSelfHandler.sendEmptyMessage(MSG_PROCESS);
            }
        }

        public void prepare(Surface surface) {
            this.mOutputSurface = surface;
            if (!isPrepared) {
                if (mSelfHandler != null) {
                    mSelfHandler.sendEmptyMessage(MSG_INIT);
                } else {
                    synchronized (RenderThread.class) {
                        if (mSelfHandler != null) {
                            mSelfHandler.sendEmptyMessage(MSG_INIT);
                        } else {
                            delayStart = true;
                        }
                    }
                }
            }
        }

        public void release() {
            isPrepared = false;
            mSelfHandler.sendEmptyMessage(MSG_RELEASE);
        }

        public void resize() {
            if(mInputSurface != null){
                mInputSurface.setDefaultBufferSize(mSurfaceWidth,mSurfaceHeight);
                if(mListener != null){
                    mListener.onInputSurfaceChanged(mSurfaceWidth,mSurfaceHeight);
                }
            }
        }
    }

    public interface FinRenderListener {

        void onRenderPrepared(boolean isPrepared, SurfaceTexture inputSurface, int texName, long eglContext, int surfaceWidth, int surfaceHeight);

        void onFrameRendered();

        void onInputSurfaceChanged(int surfaceWidth, int surfaceHeight);
    }

    private native long nativeCreate(Surface output);

    private native void nativeRenderOut(long engine, SurfaceTexture input);

    private native void nativeRelease(long engine);

    private native int nativeGetInputTex(long engine);

    private native long nativeGetEglContext(long engine);
}

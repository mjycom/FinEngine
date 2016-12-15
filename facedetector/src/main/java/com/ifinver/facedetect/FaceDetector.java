package com.ifinver.facedetect;

import android.content.Context;
import android.util.Log;

import java.io.File;
import java.io.FileOutputStream;
import java.io.InputStream;

/**
 * Created by iFinVer on 2016/12/15.
 * ilzq@foxmail.com
 */

public class FaceDetector {
    static{
        System.loadLibrary("face-detect-lib");
    }
    private static final String TAG = "FaceDetector";

    private static boolean initialized = false;

    public static boolean init(Context ctx){
        if(!initialized) {
            //检查文件
            File trackFile = new File(ctx.getFilesDir()+"/track_data.dat");
            if(!trackFile.exists()){
                //不存在了
                try {
                    InputStream in = ctx.getAssets().open("track_data.dat");
                    FileOutputStream fos = new FileOutputStream(trackFile);
                    byte[] buffer = new byte[1024];
                    int readCount;
                    while ((readCount = in.read(buffer)) != -1){
                        fos.write(buffer,0,readCount);
                    }
                    in.close();
                    fos.close();
                }catch (Exception ignored){
                    Log.e(TAG, "人脸检测初始化失败!无法操作track_data文件");
                    return false;
                }
            }

            int ret = nativeInit(ctx,trackFile.getAbsolutePath());
            if (ret != 0) {
                Log.e(TAG, "人脸检测初始化失败!");
                initialized = false;
            }else{
                initialized = true;
            }
        }else{
            Log.e(TAG,"人脸检测已经初始化过了");
        }
        return initialized;
    }

    public static long process(byte[] data,int width,int height){
        return nativeProcess(data,width,height);
    }

    public static void release(){
        nativeRelease();
    }



    private static native int nativeInit(Context ctx, String absolutePath);

    private static native long nativeProcess(byte[] data, int width, int height);

    private static native void nativeRelease();

}

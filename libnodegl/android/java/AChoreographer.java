package org.nodegl;

import android.os.HandlerThread;
import android.view.Choreographer;
import android.os.Process;
import java.util.concurrent.CountDownLatch;

public class AChoreographer implements Choreographer.FrameCallback {
    private static final String TAG = "Achoreographer";

    private long nativePtr;
    private Choreographer choreographer;
    private CountDownLatch initLatch = new CountDownLatch(1);
    private HandlerThread thread;
    private boolean started;

    public AChoreographer(long nativePtr) {

        this.nativePtr = nativePtr;

        this.thread = new HandlerThread(TAG, Process.THREAD_PRIORITY_DISPLAY) {
            protected void onLooperPrepared() {
                choreographer = Choreographer.getInstance();
                initLatch.countDown();
            }
        };
        this.thread.start();

        try {
            initLatch.await();
        } catch (InterruptedException e) {
            /* pass */
        }
    }

    public void start() {
        synchronized (this) {
            started = true;
            choreographer.postFrameCallback(this);
        }
    }

    public void stop() {
        synchronized (this) {
            started = false;
            choreographer.removeFrameCallback(this);
        }
    }

    public void release() {
        synchronized (this) {
            started = false;
            choreographer.removeFrameCallback(this);
        }
        thread.quitSafely();
    }

    @Override
    public synchronized void doFrame(long frameTimeNanos) {
        if (started) {
            choreographer.postFrameCallback(this);
            nativeDoFrame(nativePtr, frameTimeNanos);
        }
    }

    public native void nativeDoFrame(long nativePtr, long frameTimeNanos);
}
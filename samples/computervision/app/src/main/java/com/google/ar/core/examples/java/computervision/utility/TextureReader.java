/*
 * Copyright 2017 Google Inc. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.google.ar.core.examples.java.computervision.utility;

import android.opengl.GLES11Ext;
import android.opengl.GLES20;
import android.opengl.GLES30;
import com.google.ar.core.examples.java.computervision.rendering.ShaderUtil;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.FloatBuffer;
import java.nio.IntBuffer;

/**
 * Helper class for ARCore apps to read camera image from an OpenGL OES texture.
 *
 * <p>This class provides two methods for reading pixels from a texture:
 *
 * <p>(A) All-in-one method: this method utilizes two frame buffers. It does not block the caller
 * thread. Instead it submits a reading request to read pixel to back buffer from the current
 * texture, and returns pixels from the front buffer bund to texture supplied to the previous call
 * to this function. This can be done by calling submitAndAcquire() function.
 *
 * <p>(B) Asychronous method: this method utilizes multiple frame buffers and it does not block the
 * caller thread. This method allows you to read a texture in a lower frequency than rendering
 * frequency(Calling submitAndAcquire() in a lower frequency will result in an "old" image buffer
 * that was submitted a few frames ago). This method contains three routines: submitFrame(),
 * acquireFrame() and releaseFrame().
 *
 * <p>First, you call submitFrame() to submit a frame reading request. GPU will start the reading
 * process in background:
 *
 * <p>bufferIndex = submitFrame(textureId, textureWidth, textureHeight);
 *
 * <p>Second, you call acquireFrame() to get the actual image frame:
 *
 * <p>imageBuffer = acquireFrame(bufferIndex);
 *
 * <p>Last, when you finish using of the imageBuffer retured from acquireFrame(), you need to
 * release the associated frame buffer so that you can reuse it in later frame:
 *
 * <p>releaseFrame(bufferIndex);
 *
 * <p>Note: To use any of the above two methods, you need to call create() routine to initialize the
 * reader before calling any of the reading routine. You will also need to call destroy() method to
 * release the internal resource when you are done with the reader.
 */
public class TextureReader {
    private static final String TAG = TextureReader.class.getSimpleName();

    // By default, we create only two internal buffers. So you can only hold more than one buffer
    // index in your app without releasing it. If you need to hold more than one buffers, you can
    // increase the mBufferCount member.
    private final int mBufferCount = 2;
    private int[] mFrameBuffer;
    private int[] mTexture;
    private int[] mPBO;
    private Boolean[] mBufferUsed;
    private int mFrontIndex = -1;
    private int mBackIndex = -1;

    // By default, the output image format is set to RGBA. You can also set it to IMAGE_FORMAT_I8.
    private int mImageFormat = CameraImageBuffer.IMAGE_FORMAT_RGBA;
    private int mImageWidth = 0;
    private int mImageHeight = 0;
    private int mPixelBufferSize = 0;
    private Boolean mKeepAspectRatio = false;

    private FloatBuffer mQuadVertices;
    private FloatBuffer mQuadTexCoord;
    private int mQuadProgram;
    private int mQuadPositionAttrib;
    private int mQuadTexCoordAttrib;
    private static final int COORDS_PER_VERTEX = 3;
    private static final int TEXCOORDS_PER_VERTEX = 2;
    private static final int FLOAT_SIZE = 4;
    private static final float[] QUAD_COORDS =
        new float[] {
            -1.0f, -1.0f, 0.0f,
            -1.0f, +1.0f, 0.0f,
            +1.0f, -1.0f, 0.0f,
            +1.0f, +1.0f, 0.0f,
        };

    private static final float[] QUAD_TEXCOORDS =
        new float[] {
            0.0f, 0.0f,
            0.0f, 1.0f,
            1.0f, 0.0f,
            1.0f, 1.0f,
        };

    private static final String QUAD_RENDERING_VERTEX_SHADER =
        "// Vertex shader.\n"
            + "attribute vec4 a_Position;\n"
            + "attribute vec2 a_TexCoord;\n"
            + "varying vec2 v_TexCoord;\n"
            + "void main() {\n"
            + "   gl_Position = a_Position;\n"
            + "   v_TexCoord = a_TexCoord;\n"
            + "}";

    private static final String QUAD_RENDERING_FRAGMENT_SHADER_RGBA =
        "// Fragment shader that renders to a RGBA texture.\n"
            + "#extension GL_OES_EGL_image_external : require\n"
            + "precision mediump float;\n"
            + "varying vec2 v_TexCoord;\n"
            + "uniform samplerExternalOES sTexture;\n"
            + "void main() {\n"
            + "    gl_FragColor = texture2D(sTexture, v_TexCoord);\n"
            + "}";

    private static final String QUAD_RENDERING_FRAGMENT_SHADER_I8 =
        "// Fragment shader that renders to a grayscale texture.\n"
            + "#extension GL_OES_EGL_image_external : require\n"
            + "precision mediump float;\n"
            + "varying vec2 v_TexCoord;\n"
            + "uniform samplerExternalOES sTexture;\n"
            + "void main() {\n"
            + "    vec4 color = texture2D(sTexture, v_TexCoord);\n"
            + "    gl_FragColor.r = color.r * 0.299 + color.g * 0.587 + color.b * 0.114;\n"
            + "}";

    /**
     * Creates the texture reader.
     * This function needs to be called from the OpenGL rendering thread.
     *
     * @param format the format of the output pixel buffer. It can be one of the two values:
     *     CameraImageBuffer.IMAGE_FORMAT_RGBA or CameraImageBuffer.IMAGE_FORMAT_I8.
     * @param width the width of the output image.
     * @param height the height of the output image.
     * @param keepAspectRatio whether or not to keep aspect ratio. If true, the output image may be
     *     cropped if the image aspect ratio is different from the texture aspect ratio. If false,
     *     the output image covers the entire texture scope and no cropping is applied.
     */
    public void create(int format, int width, int height, Boolean keepAspectRatio) {
        if (format != CameraImageBuffer.IMAGE_FORMAT_RGBA
            && format != CameraImageBuffer.IMAGE_FORMAT_I8) {
            throw new RuntimeException("Image format not supported.");
        }

        mKeepAspectRatio = keepAspectRatio;
        mImageFormat = format;
        mImageWidth = width;
        mImageHeight = height;
        mFrontIndex = -1;
        mBackIndex = -1;

        if (mImageFormat == CameraImageBuffer.IMAGE_FORMAT_RGBA) {
            mPixelBufferSize = mImageWidth * mImageHeight * 4;
        } else if (mImageFormat == CameraImageBuffer.IMAGE_FORMAT_I8) {
            mPixelBufferSize = mImageWidth * mImageHeight;
        }

        // Create framebuffers and PBOs.
        mPBO = new int[mBufferCount];
        mFrameBuffer = new int[mBufferCount];
        mTexture = new int[mBufferCount];
        mBufferUsed = new Boolean[mBufferCount];
        GLES30.glGenBuffers(mBufferCount, mPBO, 0);
        GLES20.glGenFramebuffers(mBufferCount, mFrameBuffer, 0);
        GLES20.glGenTextures(mBufferCount, mTexture, 0);

        for (int i = 0; i < mBufferCount; i++) {
            mBufferUsed[i] = false;
            GLES20.glBindFramebuffer(GLES20.GL_FRAMEBUFFER, mFrameBuffer[i]);

            GLES20.glBindTexture(GLES20.GL_TEXTURE_2D, mTexture[i]);
            GLES30.glTexImage2D(
                GLES30.GL_TEXTURE_2D,
                0,
                mImageFormat == CameraImageBuffer.IMAGE_FORMAT_I8 ? GLES30.GL_R8 : GLES30.GL_RGBA,
                mImageWidth,
                mImageHeight,
                0,
                mImageFormat == CameraImageBuffer.IMAGE_FORMAT_I8 ? GLES30.GL_RED : GLES30.GL_RGBA,
                GLES30.GL_UNSIGNED_BYTE,
                null);
            GLES20.glTexParameteri(
                GLES20.GL_TEXTURE_2D, GLES20.GL_TEXTURE_WRAP_S, GLES20.GL_CLAMP_TO_EDGE);
            GLES20.glTexParameteri(
                GLES20.GL_TEXTURE_2D, GLES20.GL_TEXTURE_WRAP_T, GLES20.GL_CLAMP_TO_EDGE);
            GLES20.glTexParameteri(GLES20.GL_TEXTURE_2D, GLES20.GL_TEXTURE_MAG_FILTER,
                GLES20.GL_LINEAR);
            GLES20.glTexParameteri(GLES20.GL_TEXTURE_2D, GLES20.GL_TEXTURE_MIN_FILTER,
                GLES20.GL_LINEAR);
            GLES20.glFramebufferTexture2D(
                GLES20.GL_FRAMEBUFFER, GLES20.GL_COLOR_ATTACHMENT0, GLES20.GL_TEXTURE_2D,
                mTexture[i], 0);

            int status = GLES20.glCheckFramebufferStatus(GLES20.GL_FRAMEBUFFER);
            if (status != GLES20.GL_FRAMEBUFFER_COMPLETE) {
                throw new RuntimeException(
                    this
                        + ": Failed to set up render buffer with status "
                        + status
                        + " and error "
                        + GLES20.glGetError());
            }

            // Setup PBOs
            GLES30.glBindBuffer(GLES30.GL_PIXEL_PACK_BUFFER, mPBO[i]);
            GLES30.glBufferData(
                GLES30.GL_PIXEL_PACK_BUFFER, mPixelBufferSize, null, GLES30.GL_DYNAMIC_READ);
            GLES30.glBindBuffer(GLES30.GL_PIXEL_PACK_BUFFER, 0);
        }

        // Load shader program.
        int numVertices = 4;
        if (numVertices != QUAD_COORDS.length / COORDS_PER_VERTEX) {
            throw new RuntimeException("Unexpected number of vertices in BackgroundRenderer.");
        }

        ByteBuffer bbVertices = ByteBuffer.allocateDirect(QUAD_COORDS.length * FLOAT_SIZE);
        bbVertices.order(ByteOrder.nativeOrder());
        mQuadVertices = bbVertices.asFloatBuffer();
        mQuadVertices.put(QUAD_COORDS);
        mQuadVertices.position(0);

        ByteBuffer bbTexCoords =
            ByteBuffer.allocateDirect(numVertices * TEXCOORDS_PER_VERTEX * FLOAT_SIZE);
        bbTexCoords.order(ByteOrder.nativeOrder());
        mQuadTexCoord = bbTexCoords.asFloatBuffer();
        mQuadTexCoord.put(QUAD_TEXCOORDS);
        mQuadTexCoord.position(0);

        int vertexShader =
            ShaderUtil.loadGLShader(TAG, GLES20.GL_VERTEX_SHADER, QUAD_RENDERING_VERTEX_SHADER);
        int fragmentShader =
            ShaderUtil.loadGLShader(
                TAG,
                GLES20.GL_FRAGMENT_SHADER,
                mImageFormat == CameraImageBuffer.IMAGE_FORMAT_I8
                    ? QUAD_RENDERING_FRAGMENT_SHADER_I8
                    : QUAD_RENDERING_FRAGMENT_SHADER_RGBA);

        mQuadProgram = GLES20.glCreateProgram();
        GLES20.glAttachShader(mQuadProgram, vertexShader);
        GLES20.glAttachShader(mQuadProgram, fragmentShader);
        GLES20.glLinkProgram(mQuadProgram);
        GLES20.glUseProgram(mQuadProgram);

        mQuadPositionAttrib = GLES20.glGetAttribLocation(mQuadProgram, "a_Position");
        mQuadTexCoordAttrib = GLES20.glGetAttribLocation(mQuadProgram, "a_TexCoord");
        int texLoc = GLES20.glGetUniformLocation(mQuadProgram, "sTexture");
        GLES20.glUniform1i(texLoc, 0);
    }

    /** Destroy the texture reader. */
    public void destroy() {
        if (mFrameBuffer != null) {
            GLES20.glDeleteFramebuffers(mBufferCount, mFrameBuffer, 0);
            mFrameBuffer = null;
        }
        if (mTexture != null) {
            GLES20.glDeleteTextures(mBufferCount, mTexture, 0);
            mTexture = null;
        }
        if (mPBO != null) {
            GLES30.glDeleteBuffers(mBufferCount, mPBO, 0);
            mPBO = null;
        }
    }

    /**
     * Submits a frame reading request. This routine does not return the result frame buffer
     * immediately. Instead, it returns a frame buffer index, which can be used to acquire the frame
     * buffer later through acquireFrame().
     *
     * <p>If there is no enough frame buffer available, an exception will be thrown.
     *
     * @param textureId the id of the input OpenGL texture.
     * @param textureWidth width of the texture in pixels.
     * @param textureHeight height of the texture in pixels.
     * @return the index to the frame buffer this request is associated to. You should use this
     *     index to acquire the frame using acquireFrame(); and you should release the frame buffer
     *     using releaseBuffer() routine after using of the frame.
     */
    public int submitFrame(int textureId, int textureWidth, int textureHeight) {
        // Find next buffer.
        int bufferIndex = -1;
        for (int i = 0; i < mBufferCount; i++) {
            if (!mBufferUsed[i]) {
                bufferIndex = i;
                break;
            }
        }
        if (bufferIndex == -1) {
            throw new RuntimeException("No buffer available.");
        }

        // Bind both read and write to framebuffer.
        GLES20.glBindFramebuffer(GLES20.GL_FRAMEBUFFER, mFrameBuffer[bufferIndex]);

        // Save and setup viewport
        IntBuffer viewport = IntBuffer.allocate(4);
        GLES20.glGetIntegerv(GLES20.GL_VIEWPORT, viewport);
        GLES20.glViewport(0, 0, mImageWidth, mImageHeight);

        // Draw texture to framebuffer.
        drawTexture(textureId, textureWidth, textureHeight);

        // Start reading into PBO
        GLES30.glBindBuffer(GLES30.GL_PIXEL_PACK_BUFFER, mPBO[bufferIndex]);
        GLES30.glReadBuffer(GLES30.GL_COLOR_ATTACHMENT0);

        GLES30.glReadPixels(
            0,
            0,
            mImageWidth,
            mImageHeight,
            mImageFormat == CameraImageBuffer.IMAGE_FORMAT_I8 ? GLES30.GL_RED : GLES20.GL_RGBA,
            GLES20.GL_UNSIGNED_BYTE,
            0);

        // Restore viewport.
        GLES20.glViewport(viewport.get(0), viewport.get(1), viewport.get(2), viewport.get(3));

        GLES20.glBindFramebuffer(GLES20.GL_FRAMEBUFFER, 0);
        GLES30.glBindBuffer(GLES30.GL_PIXEL_PACK_BUFFER, 0);

        mBufferUsed[bufferIndex] = true;
        return bufferIndex;
    }

    /**
     * Acquires the frame requested earlier. This routine returns a CameraImageBuffer object that
     * contains the pixels mapped to the frame buffer requested previously through submitFrame().
     *
     * <p>If input buffer index is invalid, an exception will be thrown.
     *
     * @param bufferIndex the index to the frame buffer to be acquired. It has to be a frame index
     *     returned from submitFrame().
     * @return a CameraImageBuffer object if succeed. Null otherwise.
     */
    public CameraImageBuffer acquireFrame(int bufferIndex) {
        if (bufferIndex < 0 || bufferIndex >= mBufferCount || !mBufferUsed[bufferIndex]) {
            throw new RuntimeException("Invalid buffer index.");
        }

        // Bind the current PB and acquire the pixel buffer.
        GLES30.glBindBuffer(GLES30.GL_PIXEL_PACK_BUFFER, mPBO[bufferIndex]);
        ByteBuffer mapped =
            (ByteBuffer)
                GLES30.glMapBufferRange(
                    GLES30.GL_PIXEL_PACK_BUFFER, 0, mPixelBufferSize, GLES30.GL_MAP_READ_BIT);

        // Wrap the mapped buffer into CameraImageBuffer object.
        CameraImageBuffer buffer =
            new CameraImageBuffer(mImageWidth, mImageHeight, mImageFormat, mapped);

        return buffer;
    }

    /**
     * Releases a previously requested frame buffer. If input buffer index is invalid, an exception
     * will be thrown.
     *
     * @param bufferIndex the index to the frame buffer to be acquired. It has to be a frame index
     *     returned from submitFrame().
     */
    public void releaseFrame(int bufferIndex) {
        if (bufferIndex < 0 || bufferIndex >= mBufferCount || !mBufferUsed[bufferIndex]) {
            throw new RuntimeException("Invalid buffer index.");
        }
        GLES30.glBindBuffer(GLES30.GL_PIXEL_PACK_BUFFER, mPBO[bufferIndex]);
        GLES30.glUnmapBuffer(GLES30.GL_PIXEL_PACK_BUFFER);
        GLES30.glBindBuffer(GLES30.GL_PIXEL_PACK_BUFFER, 0);
        mBufferUsed[bufferIndex] = false;
    }

    /**
     * Reads pixels using dual buffers. This function sends the reading request to GPU and returns
     * the result from the previous call. Thus, the first call always returns null. The pixelBuffer
     * member in the returned object maps to the internal buffer. This buffer cannot be overrode,
     * and it becomes invalid after next call to submitAndAcquire().
     *
     * @param textureId the OpenGL texture Id.
     * @param textureWidth width of the texture in pixels.
     * @param textureHeight height of the texture in pixels.
     * @return a CameraImageBuffer object that contains the pixels read from the texture.
     */
    public CameraImageBuffer submitAndAcquire(int textureId, int textureWidth, int textureHeight) {
        // Release previously used front buffer.
        if (mFrontIndex != -1) {
            releaseFrame(mFrontIndex);
        }

        // Move previous back buffer to front buffer.
        mFrontIndex = mBackIndex;

        // Submit new request on back buffer.
        mBackIndex = submitFrame(textureId, textureWidth, textureHeight);

        // Acquire frame from the new front buffer.
        if (mFrontIndex != -1) {
            return acquireFrame(mFrontIndex);
        }

        return null;
    }

    /** Draws texture to full screen. */
    private void drawTexture(int textureId, int textureWidth, int textureHeight) {
        // Disable features that we don't use.
        GLES20.glDisable(GLES20.GL_DEPTH_TEST);
        GLES20.glDisable(GLES20.GL_CULL_FACE);
        GLES20.glDisable(GLES20.GL_SCISSOR_TEST);
        GLES20.glDisable(GLES20.GL_STENCIL_TEST);
        GLES20.glDisable(GLES20.GL_BLEND);
        GLES20.glDepthMask(false);
        GLES20.glBindBuffer(GLES20.GL_ARRAY_BUFFER, 0);
        GLES20.glBindBuffer(GLES20.GL_ELEMENT_ARRAY_BUFFER, 0);
        GLES30.glBindVertexArray(0);

        // Clear buffers.
        GLES20.glClearColor(0, 0, 0, 0);
        GLES20.glClear(GLES20.GL_COLOR_BUFFER_BIT | GLES20.GL_DEPTH_BUFFER_BIT);

        // Set the vertex positions.
        GLES20.glVertexAttribPointer(
            mQuadPositionAttrib, COORDS_PER_VERTEX, GLES20.GL_FLOAT, false, 0, mQuadVertices);

        // Calculate the texture coordinates.
        if (mKeepAspectRatio) {
            int renderWidth = 0;
            int renderHeight = 0;
            float textureAspectRatio = (float) (textureWidth) / textureHeight;
            float imageAspectRatio = (float) (mImageWidth) / mImageHeight;
            if (textureAspectRatio < imageAspectRatio) {
                renderWidth = mImageWidth;
                renderHeight = textureHeight * mImageWidth / textureWidth;
            } else {
                renderWidth = textureWidth * mImageHeight / textureHeight;
                renderHeight = mImageHeight;
            }
            float offsetU = (float) (renderWidth - mImageWidth) / renderWidth / 2;
            float offsetV = (float) (renderHeight - mImageHeight) / renderHeight / 2;

            float[] texCoords =
                new float[] {
                    offsetU, offsetV,
                    offsetU, 1 - offsetV,
                    1 - offsetU, offsetV,
                    1 - offsetU, 1 - offsetV
                };

            mQuadTexCoord.put(texCoords);
            mQuadTexCoord.position(0);
        } else {
            mQuadTexCoord.put(QUAD_TEXCOORDS);
            mQuadTexCoord.position(0);
        }

        // Set the texture coordinates.
        GLES20.glVertexAttribPointer(
            mQuadTexCoordAttrib, TEXCOORDS_PER_VERTEX, GLES20.GL_FLOAT, false, 0, mQuadTexCoord);

        // Enable vertex arrays
        GLES20.glEnableVertexAttribArray(mQuadPositionAttrib);
        GLES20.glEnableVertexAttribArray(mQuadTexCoordAttrib);

        GLES20.glUseProgram(mQuadProgram);

        // Select input texture.
        GLES20.glActiveTexture(GLES20.GL_TEXTURE0);
        GLES20.glBindTexture(GLES11Ext.GL_TEXTURE_EXTERNAL_OES, textureId);

        // Draw a quad with texture.
        GLES20.glDrawArrays(GLES20.GL_TRIANGLE_STRIP, 0, 4);

        // Disable vertex arrays
        GLES20.glDisableVertexAttribArray(mQuadPositionAttrib);
        GLES20.glDisableVertexAttribArray(mQuadTexCoordAttrib);

        // Reset texture binding.
        GLES20.glBindTexture(GLES11Ext.GL_TEXTURE_EXTERNAL_OES, 0);
    }
}

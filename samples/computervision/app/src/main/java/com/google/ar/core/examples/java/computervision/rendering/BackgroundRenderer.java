/*
 * Copyright 2017 Google Inc. All Rights Reserved.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
package com.google.ar.core.examples.java.computervision.rendering;

import android.content.Context;
import android.opengl.GLES11Ext;
import android.opengl.GLES20;
import android.opengl.GLSurfaceView;
import com.google.ar.core.Frame;
import com.google.ar.core.Session;
import com.google.ar.core.examples.java.computervision.R;
import com.google.ar.core.examples.java.computervision.utility.CameraImageBuffer;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.FloatBuffer;
import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.opengles.GL10;

/**
 * This class renders the AR background from camera feed. It creates and hosts the texture given to
 * ARCore to be filled with the camera image.
 */
public class BackgroundRenderer {
    private static final String TAG = BackgroundRenderer.class.getSimpleName();

    private static final int COORDS_PER_VERTEX = 3;
    private static final int TEXCOORDS_PER_VERTEX = 2;
    private static final int FLOAT_SIZE = 4;

    private FloatBuffer mQuadVertices;
    private FloatBuffer mQuadTexCoord;
    private FloatBuffer mQuadTexCoordTransformed;

    private int mQuadProgram;

    private int mQuadPositionAttrib;
    private int mQuadTexCoordAttrib;
    private int mQuadSplitterUniform;
    private int mBackgroundTextureId = -1;
    private int mOverlayTextureId = -1;
    private float mSplitterPosition = 0.5f;

    private CameraImageBuffer mOverlayImageBuffer;

    public int getTextureId() {
        return mBackgroundTextureId;
    }

    /**
     * Allocates and initializes OpenGL resources needed by the background renderer. Must be called
     * on the OpenGL thread, typically in {@link GLSurfaceView.Renderer#onSurfaceCreated(GL10,
     * EGLConfig)}.
     *
     * @param context Needed to access shader source.
     */
    public void createOnGlThread(Context context) {
        // Generate the background texture.
        int[] textures = new int[2];
        GLES20.glGenTextures(2, textures, 0);
        mBackgroundTextureId = textures[0];
        GLES20.glBindTexture(GLES11Ext.GL_TEXTURE_EXTERNAL_OES, mBackgroundTextureId);
        GLES20.glTexParameteri(
            GLES11Ext.GL_TEXTURE_EXTERNAL_OES, GLES20.GL_TEXTURE_WRAP_S, GLES20.GL_CLAMP_TO_EDGE);
        GLES20.glTexParameteri(
            GLES11Ext.GL_TEXTURE_EXTERNAL_OES, GLES20.GL_TEXTURE_WRAP_T, GLES20.GL_CLAMP_TO_EDGE);
        GLES20.glTexParameteri(
            GLES11Ext.GL_TEXTURE_EXTERNAL_OES, GLES20.GL_TEXTURE_MIN_FILTER, GLES20.GL_NEAREST);
        GLES20.glTexParameteri(
            GLES11Ext.GL_TEXTURE_EXTERNAL_OES, GLES20.GL_TEXTURE_MAG_FILTER, GLES20.GL_NEAREST);

        mOverlayTextureId = textures[1];
        GLES20.glBindTexture(GLES20.GL_TEXTURE_2D, mOverlayTextureId);
        GLES20.glTexParameteri(GLES20.GL_TEXTURE_2D, GLES20.GL_TEXTURE_WRAP_S,
            GLES20.GL_CLAMP_TO_EDGE);
        GLES20.glTexParameteri(GLES20.GL_TEXTURE_2D, GLES20.GL_TEXTURE_WRAP_T,
            GLES20.GL_CLAMP_TO_EDGE);
        GLES20.glTexParameteri(GLES20.GL_TEXTURE_2D, GLES20.GL_TEXTURE_MIN_FILTER,
            GLES20.GL_NEAREST);

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

        ByteBuffer bbTexCoordsTransformed =
            ByteBuffer.allocateDirect(numVertices * TEXCOORDS_PER_VERTEX * FLOAT_SIZE);
        bbTexCoordsTransformed.order(ByteOrder.nativeOrder());
        mQuadTexCoordTransformed = bbTexCoordsTransformed.asFloatBuffer();

        int vertexShader =
            ShaderUtil.loadGLShader(TAG, context, GLES20.GL_VERTEX_SHADER, R.raw.screenquad_vertex);
        int fragmentShader =
            ShaderUtil.loadGLShader(TAG, context, GLES20.GL_FRAGMENT_SHADER,
                R.raw.screenquad_fragment);

        mQuadProgram = GLES20.glCreateProgram();
        GLES20.glAttachShader(mQuadProgram, vertexShader);
        GLES20.glAttachShader(mQuadProgram, fragmentShader);
        GLES20.glLinkProgram(mQuadProgram);
        GLES20.glUseProgram(mQuadProgram);

        ShaderUtil.checkGLError(TAG, "Program creation");

        mQuadPositionAttrib = GLES20.glGetAttribLocation(mQuadProgram, "a_Position");
        mQuadTexCoordAttrib = GLES20.glGetAttribLocation(mQuadProgram, "a_TexCoord");
        mQuadSplitterUniform = GLES20.glGetUniformLocation(mQuadProgram, "s_SplitterPosition");

        int texLoc = GLES20.glGetUniformLocation(mQuadProgram, "TexVideo");
        GLES20.glUniform1i(texLoc, 0);
        texLoc = GLES20.glGetUniformLocation(mQuadProgram, "TexImage");
        GLES20.glUniform1i(texLoc, 1);

        ShaderUtil.checkGLError(TAG, "Program parameters");
    }

    /**
     * Sets new overlay image buffer. This image buffer is used to render side by side with
     * background video.
     *
     * @param imageBuffer the new image buffer for the overlay texture.
     */
    public void setOverlayImage(CameraImageBuffer imageBuffer) {
        mOverlayImageBuffer = imageBuffer;
    }

    /**
     * Gets the texture splitter position.
     *
     * @return the splitter position.
     */
    public float getSplitterPosition() {
        return mSplitterPosition;
    }

    /**
     * Sets the splitter position. This position defines the splitting position between the
     * background video and the image.
     *
     * @param position the new splitter position.
     */
    public void setSplitterPosition(float position) {
        mSplitterPosition = position;
    }

    /**
     * Draws the AR background image. The image will be drawn such that virtual content rendered
     * with the matrices provided by {@link Frame#getViewMatrix(float[], int)} and {@link
     * Session#getProjectionMatrix(float[], int, float, float)} will accurately follow static
     * physical objects. This must be called <b>before</b> drawing virtual content.
     *
     * @param frame The last {@code Frame} returned by {@link Session#update()}.
     */
    public void draw(Frame frame) {
        // If display rotation changed (also includes view size change), we need to re-query the uv
        // coordinates for the screen rect, as they may have changed as well.
        if (frame.hasDisplayGeometryChanged()) {
            frame.transformDisplayUvCoords(mQuadTexCoord, mQuadTexCoordTransformed);
        }

        // No need to test or write depth, the screen quad has arbitrary depth, and is expected
        // to be drawn first.
        GLES20.glDisable(GLES20.GL_DEPTH_TEST);
        GLES20.glDepthMask(false);

        GLES20.glActiveTexture(GLES20.GL_TEXTURE0);
        GLES20.glBindTexture(GLES11Ext.GL_TEXTURE_EXTERNAL_OES, mBackgroundTextureId);

        GLES20.glActiveTexture(GLES20.GL_TEXTURE1);

        // Apply overlay image buffer
        if (mOverlayImageBuffer != null
            && mOverlayImageBuffer.format == CameraImageBuffer.IMAGE_FORMAT_I8) {
            GLES20.glBindTexture(GLES20.GL_TEXTURE_2D, mOverlayTextureId);

            ByteBuffer pixelBuffer = mOverlayImageBuffer.buffer;
            pixelBuffer.position(0);
            GLES20.glTexImage2D(
                GLES20.GL_TEXTURE_2D,
                0,
                GLES20.GL_LUMINANCE,
                mOverlayImageBuffer.width,
                mOverlayImageBuffer.height,
                0,
                GLES20.GL_LUMINANCE,
                GLES20.GL_UNSIGNED_BYTE,
                pixelBuffer);
        }

        GLES20.glUseProgram(mQuadProgram);

        // Set the vertex positions.
        GLES20.glVertexAttribPointer(
            mQuadPositionAttrib, COORDS_PER_VERTEX, GLES20.GL_FLOAT, false, 0, mQuadVertices);

        // Set splitter position.
        GLES20.glUniform1f(mQuadSplitterUniform, mSplitterPosition);

        // Set the texture coordinates.
        GLES20.glVertexAttribPointer(
            mQuadTexCoordAttrib,
            TEXCOORDS_PER_VERTEX,
            GLES20.GL_FLOAT,
            false,
            0,
            mQuadTexCoordTransformed);

        // Enable vertex arrays
        GLES20.glEnableVertexAttribArray(mQuadPositionAttrib);
        GLES20.glEnableVertexAttribArray(mQuadTexCoordAttrib);

        GLES20.glDrawArrays(GLES20.GL_TRIANGLE_STRIP, 0, 4);

        // Disable vertex arrays
        GLES20.glDisableVertexAttribArray(mQuadPositionAttrib);
        GLES20.glDisableVertexAttribArray(mQuadTexCoordAttrib);

        // Restore the depth state for further drawing.
        GLES20.glDepthMask(true);
        GLES20.glEnable(GLES20.GL_DEPTH_TEST);

        ShaderUtil.checkGLError(TAG, "Draw");
    }

    private static final float[] QUAD_COORDS =
        new float[] {
            -1.0f, -1.0f, 0.0f,
            -1.0f, +1.0f, 0.0f,
            +1.0f, -1.0f, 0.0f,
            +1.0f, +1.0f, 0.0f,
        };

    private static final float[] QUAD_TEXCOORDS =
        new float[] {
            0.0f, 1.0f,
            0.0f, 0.0f,
            1.0f, 1.0f,
            1.0f, 0.0f,
        };
}

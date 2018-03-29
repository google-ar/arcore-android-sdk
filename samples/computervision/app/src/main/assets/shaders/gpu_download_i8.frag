// Fragment shader that renders to a grayscale texture.
#extension GL_OES_EGL_image_external : require

precision mediump float;
varying vec2 v_TexCoord;
uniform samplerExternalOES sTexture;

void main() {
    vec4 color = texture2D(sTexture, v_TexCoord);
    gl_FragColor.r = color.r * 0.299 + color.g * 0.587 + color.b * 0.114;
}

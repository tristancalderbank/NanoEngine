#pragma once

/**
 * Framebuffer for rendering bloom.
 */
class BloomFramebuffer {
public:
    BloomFramebuffer(int width, int height);
    void init();
    void bind();
    void resize(int width, int height);
    void setMipLevel(int mipLevel);
    unsigned int getColorTextureId();

private:
	int mWidth, mHeight;
    int mMipLevel = 0;
	unsigned int mFramebufferId;
	unsigned int mColorTextureId;
};

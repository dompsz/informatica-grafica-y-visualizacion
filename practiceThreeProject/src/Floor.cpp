#include "Floor.h"

Floor::Floor(float size) : _size(size), currentMaterialIndex(0), textureEnabled(true), currentTextureIndex(0) {
    createMaterials();
    // loadTextures() is now called from init()
}

void Floor::init() {
    loadTextures();
}

void Floor::createMaterials() {
    // Rubber
    GLfloat amb1[] = {0.02f, 0.02f, 0.02f, 1.0f};
    GLfloat diff1[] = {0.01f, 0.01f, 0.01f, 1.0f};
    GLfloat spec1[] = {0.4f, 0.4f, 0.4f, 1.0f};
    materials.emplace_back(amb1, diff1, spec1, 10.0f);

    // Plastic
    GLfloat amb2[] = {0.0f, 0.0f, 0.0f, 1.0f};
    GLfloat diff2[] = {0.55f, 0.55f, 0.55f, 1.0f};
    GLfloat spec2[] = {0.7f, 0.7f, 0.7f, 1.0f};
    materials.emplace_back(amb2, diff2, spec2, 32.0f);

    // Polished Metal
    GLfloat amb3[] = {0.25f, 0.25f, 0.25f, 1.0f};
    GLfloat diff3[] = {0.4f, 0.4f, 0.4f, 1.0f};
    GLfloat spec3[] = {0.77f, 0.77f, 0.77f, 1.0f};
    materials.emplace_back(amb3, diff3, spec3, 76.8f);
}

void Floor::loadTextures() {
    textures.push_back(std::make_unique<Texture>());
    textures.back()->load("textures/grid.png");

    textures.push_back(std::make_unique<Texture>());
    textures.back()->load("textures/water.png");

    textures.push_back(std::make_unique<Texture>());
    textures.back()->load("textures/bricks.png");
}

void Floor::setMaterial(int materialIndex) {
    if (materialIndex >= 0 && materialIndex < materials.size()) {
        currentMaterialIndex = materialIndex;
    }
}

void Floor::toggleTexture(bool enable) {
    textureEnabled = enable;
}

void Floor::setTexture(int textureIndex) {
    if (textureIndex >= 0 && textureIndex < textures.size()) {
        currentTextureIndex = textureIndex;
    }
}

void Floor::setTextureFilters(int filterType) {
    GLint minFilter, magFilter;
    switch (filterType) {
        case 0: minFilter = GL_NEAREST; magFilter = GL_NEAREST; break;
        case 1: minFilter = GL_LINEAR; magFilter = GL_NEAREST; break;
        case 2: minFilter = GL_NEAREST; magFilter = GL_LINEAR; break;
        case 3: default: minFilter = GL_LINEAR; magFilter = GL_LINEAR; break;
    }
    if (currentTextureIndex >= 0 && currentTextureIndex < textures.size()) {
        textures[currentTextureIndex]->setFilters(minFilter, magFilter);
    }
}

void Floor::draw() {
    glPushMatrix();
    applyTransformations();

    materials[currentMaterialIndex].apply();

    if (textureEnabled && currentTextureIndex < textures.size()) {
        glEnable(GL_TEXTURE_2D);
        textures[currentTextureIndex]->bind();
    } else {
        glDisable(GL_TEXTURE_2D);
    }

    glBegin(GL_QUADS);
    glNormal3f(0.0f, 1.0f, 0.0f);
    
    glTexCoord2f(0.0f, 0.0f); glVertex3f(-_size / 2, 0.0f, -_size / 2);
    glTexCoord2f(0.0f, 1.0f); glVertex3f(-_size / 2, 0.0f, _size / 2);
    glTexCoord2f(1.0f, 1.0f); glVertex3f(_size / 2, 0.0f, _size / 2);
    glTexCoord2f(1.0f, 0.0f); glVertex3f(_size / 2, 0.0f, -_size / 2);
    
    glEnd();

    if (textureEnabled) {
        textures[currentTextureIndex]->unbind();
        glDisable(GL_TEXTURE_2D);
    }

    glPopMatrix();
}

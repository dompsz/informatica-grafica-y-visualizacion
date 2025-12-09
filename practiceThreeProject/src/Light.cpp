#include "Light.h"
#include <cstring>

Light::Light(LightType t, int gl_light_num) : type(t), gl_light(gl_light_num), enabled(true), cutoff(45.0f), exponent(0.0f) {
    GLfloat def_amb[] = {0.0f, 0.0f, 0.0f, 1.0f};
    GLfloat def_diff[] = {1.0f, 1.0f, 1.0f, 1.0f};
    GLfloat def_spec[] = {1.0f, 1.0f, 1.0f, 1.0f};
    
    memcpy(ambient, def_amb, sizeof(ambient));
    memcpy(diffuse, def_diff, sizeof(diffuse));
    memcpy(specular, def_spec, sizeof(specular));
    
    setPosition(0, 5, 5);
    w_coord = 1.0f;

    if (type == AMBIENT) {
        GLfloat amb[] = {0.2f, 0.2f, 0.2f, 1.0f};
        memcpy(ambient, amb, sizeof(ambient));
        enabled = false;
    }
    if (type == DIRECTIONAL) {
        w_coord = 0.0f;
    }
}

void Light::toggle() {
    enabled = !enabled;
}

void Light::apply() {
    if (enabled) {
        glEnable(gl_light);
        glLightfv(gl_light, GL_AMBIENT, ambient);
        glLightfv(gl_light, GL_DIFFUSE, diffuse);
        glLightfv(gl_light, GL_SPECULAR, specular);

        float x, y, z;
        getPosition(x, y, z);
        GLfloat current_pos[4] = { x, y, z, w_coord };
        glLightfv(gl_light, GL_POSITION, current_pos);

        if (type == SPOTLIGHT) {
            glLightfv(gl_light, GL_SPOT_DIRECTION, direction);
            glLightf(gl_light, GL_SPOT_CUTOFF, cutoff);
            glLightf(gl_light, GL_SPOT_EXPONENT, exponent);
        } else {
            glLightf(gl_light, GL_SPOT_CUTOFF, 180.0f);
        }
    } else {
        glDisable(gl_light);
    }
}

void Light::draw() { // Removed const
    if (enabled && (type == POINT_LIGHT || type == SPOTLIGHT)) {
        glPushMatrix();
        applyTransformations();

        glDisable(GL_LIGHTING);
        glColor3f(1.0f, 1.0f, 0.0f); // Yellow sphere
        glutSolidSphere(0.2, 16, 16);
        glEnable(GL_LIGHTING);

        glPopMatrix();
    }
}

void Light::setAmbient(const GLfloat* amb) { memcpy(ambient, amb, sizeof(ambient)); }
void Light::setDiffuse(const GLfloat* diff) { memcpy(diffuse, diff, sizeof(diffuse)); }
void Light::setSpecular(const GLfloat* spec) { memcpy(specular, spec, sizeof(specular)); }
void Light::setPosition(float x, float y, float z) {
    resetTransformations();
    translate(x, y, z);
}
void Light::setDirection(const GLfloat* dir) { memcpy(direction, dir, sizeof(direction)); }
void Light::setCutoff(GLfloat c) { cutoff = c; }
void Light::setExponent(GLfloat e) { exponent = e; }

#ifndef LIGHT_H
#define LIGHT_H

#if defined(__APPLE__) && defined(__MACH__)
#include <GLUT/glut.h>
#else
#include <GL/glut.h>
#endif

#include "Object3D.h"

enum LightType { AMBIENT, POINT_LIGHT, DIRECTIONAL, SPOTLIGHT };

class Light : public Object3D {
public:
    Light(LightType type, int gl_light_num);

    void apply();
    void toggle();
    bool isEnabled() const { return enabled; }
    void draw() override; // Removed const, for visualization

    void setAmbient(const GLfloat* amb);
    void setDiffuse(const GLfloat* diff);
    void setSpecular(const GLfloat* spec);
    void setPosition(float x, float y, float z);
    void setDirection(const GLfloat* dir);
    void setCutoff(GLfloat cutoff);
    void setExponent(GLfloat exponent);

private:
    LightType type;
    int gl_light;
    bool enabled;

    GLfloat ambient[4];
    GLfloat diffuse[4];
    GLfloat specular[4];
    // position is now handled by Object3D's translate members
    GLfloat direction[3];
    GLfloat cutoff;
    GLfloat exponent;
    GLfloat w_coord; // To store w component for position (1.0 for point/spot, 0.0 for directional)
};

#endif // LIGHT_H

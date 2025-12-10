#include "igvInterface.h"
#include "src/AdvancedOBJLoader.h"
#include <iostream>
#include <cmath>

igvInterface* igvInterface::_instance = nullptr;

// Static callbacks
void menu_callback(int option);
void shading_menu_callback(int option);
void interaction_menu_callback(int option);
void animation_menu_callback(int option);
void material_menu_callback(int option);
void texture_menu_callback(int option);
void texture_filter_menu_callback(int option);
void light_menu_callback(int option);
void light_select_menu_callback(int option);

// Mouse and selection state
static int last_mouse_y;
static int selected_dof_by_mouse = -1;
static bool selection_requested = false;
static int selection_x, selection_y;

igvInterface& igvInterface::getInstance() {
    if (!_instance) _instance = new igvInterface;
    return *_instance;
}

igvInterface::igvInterface() {
    camera = new Camera();
    triangleMesh = new cgvTriangleMesh();
    articulatedModel = new ArticulatedModel();
    floor = new Floor();

    if (!AdvancedOBJLoader::load("objFiles/cow.obj", *triangleMesh)) {
        std::cerr << "Failed to load cow mesh." << std::endl;
    }
    triangleMesh->set_specular_reflectivity(0.1f);
    triangleMesh->set_shininess(10.0f);
    triangleMesh->translate(-5, 0, 0);

    articulatedModel->translate(5, 0, 0);
    floor->translate(0, -1.5, 0);

    selectedObject = nullptr;
    currentObject = 0;
    selectedLight = -1;
    cameraMode = false;
    flatShading = false;
    articulatedInteractionKeyboard = true;
    animateCamera = false;
    animateModel = false;
    animateLight = false; // Initialized to false
    textureEnabled = true;
    globalAmbientLightOn = true;
}

igvInterface::~igvInterface() {
    delete camera;
    delete triangleMesh;
    delete articulatedModel;
    delete floor;
}

void igvInterface::setupLights() {
    // GL_LIGHT0 - Point Light
    lights.push_back(std::make_unique<Light>(POINT_LIGHT, GL_LIGHT0));
    lights[0]->setPosition(0.0f, 5.0f, 5.0f);
    lights[0]->toggle(); // Enabled by default

    // GL_LIGHT1 - Directional
    lights.push_back(std::make_unique<Light>(DIRECTIONAL, GL_LIGHT1));
    lights[1]->setPosition(-1.0f, -1.0f, -1.0f); // Position is used as direction for directional lights
    GLfloat diff1[] = {0.8f, 0.8f, 0.8f, 1.0f};
    lights[1]->setDiffuse(diff1);

    // GL_LIGHT2 - Spotlight
    lights.push_back(std::make_unique<Light>(SPOTLIGHT, GL_LIGHT2));
    lights[2]->setPosition(5.0f, 8.0f, 5.0f);
    GLfloat dir2[] = {-1.0f, -1.0f, 0.0f};
    lights[2]->setDirection(dir2);
    GLfloat diff2[] = {1.0f, 0.5f, 0.5f, 1.0f}; // Reddish
    lights[2]->setDiffuse(diff2);
    lights[2]->setCutoff(30.0f);
}

void igvInterface::initGLResources() {
    setupLights();
    floor->init(); // Initialize floor textures
}

void igvInterface::configure_environment(int argc, char** argv, int _window_width, int _window_height, int _pos_X, int _pos_Y, std::string _title) {
    window_width = _window_width;
    window_height = _window_height;

    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH);
    glutInitWindowSize(_window_width, _window_height);
    glutInitWindowPosition(_pos_X, _pos_Y);
    glutCreateWindow(_title.c_str());

    glEnable(GL_DEPTH_TEST);
    glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
    glEnable(GL_LIGHTING);

    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT, GL_AMBIENT_AND_DIFFUSE);
    
    create_menus();
    initGLResources(); // Call after context is created
}

void igvInterface::start_display_loop() {
    glutMainLoop();
}

void igvInterface::keyboardFunc(unsigned char key, int x, int y) {
    igvInterface* i = &getInstance();
    float move_speed = 0.5f;
    switch (key) {
        case 27: exit(0);
        case 'c': case 'C': i->cameraMode = !i->cameraMode; i->selectLight(-1); break;
        case 'p': case 'P': i->camera->toggleProjection(); break;
        case '=': case '+': i->camera->zoom(-1.0f); break;
        case '_': case '-': i->camera->zoom(1.0f); break;
        case '1': i->selectObject(1); break; // Changed to 1
        case '2': i->selectObject(2); break; // Changed to 2
        
        // Light movement
        case 'j': i->moveSelectedLight(-move_speed, 0, 0); break; // Left
        case 'l': i->moveSelectedLight(move_speed, 0, 0); break;  // Right
        case 'i': i->moveSelectedLight(0, move_speed, 0); break;   // Up
        case 'k': i->moveSelectedLight(0, -move_speed, 0); break;  // Down
        case 'u': i->moveSelectedLight(0, 0, -move_speed); break; // Forward
        case 'o': i->moveSelectedLight(0, 0, move_speed); break;  // Backward

        // Robot DoF selection
        case '4': if (i->currentObject == 2) i->articulatedModel->prev_dof(); break;
        case '5': if (i->currentObject == 2) i->articulatedModel->next_dof(); break;

        // Object rotation
        case 'X': if(i->selectedObject) i->selectedObject->rotate(15.0f, 0.0f, 0.0f); break;
        case 'x': if(i->selectedObject) i->selectedObject->rotate(-15.0f, 0.0f, 0.0f); break;
        case 'Y': if(i->selectedObject) i->selectedObject->rotate(0.0f, 15.0f, 0.0f); break;
        case 'y': if(i->selectedObject) i->selectedObject->rotate(0.0f, -15.0f, 0.0f); break;
        case 'Z': if(i->selectedObject) i->selectedObject->rotate(0.0f, 0.0f, 15.0f); break;
        case 'z': if(i->selectedObject) i->selectedObject->rotate(0.0f, 0.0f, -15.0f); break;

        // Object scaling
        case 'S': if(i->selectedObject) i->selectedObject->scale(1.1f, 1.1f, 1.1f); break;
        case 's': if(i->selectedObject) i->selectedObject->scale(0.9f, 0.9f, 0.9f); break;
        
        case 'a': case 'A': i->toggleAnimateModel(); break;
        case 'g': case 'G': i->toggleAnimateCamera(); break;
        case 'b': case 'B': i->toggleAnimateLight(); break; // Shortcut for light animation
    }
    glutPostRedisplay();
}

void igvInterface::specialKeyboardFunc(int key, int x, int y) {
    igvInterface* i = &getInstance();
    float move_speed = 0.5f;
    if (i->cameraMode) {
        switch (key) {
            case GLUT_KEY_LEFT: i->camera->orbit(-5.0f, 0.0f); break;
            case GLUT_KEY_RIGHT: i->camera->orbit(5.0f, 0.0f); break;
            case GLUT_KEY_UP: i->camera->orbit(0.0f, 5.0f); break;
            case GLUT_KEY_DOWN: i->camera->orbit(0.0f, -5.0f); break;
        }
    } else if (i->currentObject == 2 && i->articulatedInteractionKeyboard) { // Robot DoF control
        switch (key) {
            case GLUT_KEY_UP: i->articulatedModel->increase_dof(); break;
            case GLUT_KEY_DOWN: i->articulatedModel->decrease_dof(); break;
        }
    } else if (i->selectedObject) { // General object movement
        switch (key) {
            case GLUT_KEY_LEFT: i->selectedObject->translate(-move_speed, 0.0f, 0.0f); break;
            case GLUT_KEY_RIGHT: i->selectedObject->translate(move_speed, 0.0f, 0.0f); break;
            case GLUT_KEY_UP: i->selectedObject->translate(0.0f, move_speed, 0.0f); break;   // Up/Down on Y-axis
            case GLUT_KEY_DOWN: i->selectedObject->translate(0.0f, -move_speed, 0.0f); break; // Up/Down on Y-axis
        }
    }
    glutPostRedisplay();
}

void igvInterface::reshapeFunc(int w, int h) {
    glViewport(0, 0, w, h);
    _instance->set_window_width(w);
    _instance->set_window_height(h);
    _instance->camera->setAspectRatio((float)w / h);
}

void igvInterface::process_selection() {
    // This is now called from displayFunc
    glDisable(GL_LIGHTING);
    glDisable(GL_DITHER);

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    camera->applyProjection();
    camera->applyView();
    articulatedModel->render_for_selection();

    unsigned char pixel[3];
    glReadPixels(selection_x, window_height - selection_y, 1, 1, GL_RGB, GL_UNSIGNED_BYTE, pixel);

    if (pixel[0] > 0 && pixel[0] <= 3) { // Assuming 1, 2, 3 are valid DoF IDs
        selected_dof_by_mouse = pixel[0] - 1; // 0-indexed DoF
        articulatedModel->set_dof(selected_dof_by_mouse);
    } else {
        selected_dof_by_mouse = -1;
    }

    glEnable(GL_LIGHTING);
    glEnable(GL_DITHER);
    selection_requested = false; // Reset flag
}

void igvInterface::displayFunc() {
    igvInterface* i = &getInstance();

    if (selection_requested) {
        i->process_selection();
    }

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    i->camera->applyProjection();
    i->camera->applyView();

    // Apply global ambient light
    GLfloat ambient_light[] = { 0.2f, 0.2f, 0.2f, 1.0f };
    if (!i->globalAmbientLightOn) {
        ambient_light[0] = 0; ambient_light[1] = 0; ambient_light[2] = 0;
    }
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, ambient_light);

    // Apply all other lights
    for (auto const& light : i->lights) {
        light->apply();
    }

    glShadeModel(i->flatShading ? GL_FLAT : GL_SMOOTH);

    // Draw axes
    glDisable(GL_LIGHTING);
    glLineWidth(1.0f);
    glBegin(GL_LINES);
    glColor3f(1.0, 0.0, 0.0); glVertex3f(-10, 0, 0); glVertex3f(10, 0, 0);
    glColor3f(0.0, 1.0, 0.0); glVertex3f(0, -10, 0); glVertex3f(0, 10, 0);
    glColor3f(0.0, 0.0, 1.0); glVertex3f(0, 0, -10); glVertex3f(0, 0, 10);
    glEnd();
    glEnable(GL_LIGHTING);

    // Draw objects
    i->triangleMesh->draw();
    i->articulatedModel->draw();
    i->floor->draw();

    // Draw light visualizations
    for (auto const& light : i->lights) {
        light->draw();
    }

    glutSwapBuffers();
}

void igvInterface::idleFunc() {
    static float last_time = 0;
    float current_time = glutGet(GLUT_ELAPSED_TIME) / 1000.0f;
    if (last_time == 0) last_time = current_time;
    float delta_time = current_time - last_time;
    last_time = current_time;

    igvInterface* i = &getInstance();
    if (i->animateModel) i->articulatedModel->update(current_time);
    if (i->animateCamera) i->camera->orbit(delta_time * 10.0f, 0);

    if (i->animateLight) {
        if (!i->lights.empty()) {
            float radius = 7.0f;
            float speed = 0.5f;
            float light_x = sin(current_time * speed) * radius;
            float light_z = cos(current_time * speed) * radius;
            i->lights[0]->setPosition(light_x, 5.0f, light_z);
        }
    }

    glutPostRedisplay();
}

void igvInterface::mouseFunc(int button, int state, int x, int y) {
    igvInterface* i = &getInstance();
    if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN) {
        if (!i->articulatedInteractionKeyboard) {
            selection_requested = true;
            selection_x = x;
            selection_y = y;
            last_mouse_y = y;
            glutPostRedisplay(); // Request a redraw to process selection
        }
    }
    if (button == GLUT_LEFT_BUTTON && state == GLUT_UP) {
        selected_dof_by_mouse = -1;
    }
}

void igvInterface::motionFunc(int x, int y) {
    igvInterface* i = &getInstance();
    if (selected_dof_by_mouse != -1) {
        float dy = y - last_mouse_y;
        if (dy > 0) i->articulatedModel->decrease_dof();
        if (dy < 0) i->articulatedModel->increase_dof();
        last_mouse_y = y;
        glutPostRedisplay();
    }
}

void igvInterface::create_menus() {
    int shading_menu = glutCreateMenu(shading_menu_callback);
    glutAddMenuEntry("Flat", 1);
    glutAddMenuEntry("Smooth", 2);

    int interaction_menu = glutCreateMenu(interaction_menu_callback);
    glutAddMenuEntry("Keyboard", 1);
    glutAddMenuEntry("Mouse (Picking)", 2);

    int animation_menu = glutCreateMenu(animation_menu_callback);
    glutAddMenuEntry("Toggle Model Animation", 1);
    glutAddMenuEntry("Toggle Camera Animation", 2);
    glutAddMenuEntry("Toggle Light Animation", 3); // Added light animation menu

    int material_menu = glutCreateMenu(material_menu_callback);
    glutAddMenuEntry("Rubber", 1);
    glutAddMenuEntry("Plastic", 2);
    glutAddMenuEntry("Metal", 3);

    int texture_filter_menu = glutCreateMenu(texture_filter_menu_callback);
    glutAddMenuEntry("Nearest, Nearest", 1);
    glutAddMenuEntry("Linear, Nearest", 2);
    glutAddMenuEntry("Nearest, Linear", 3);
    glutAddMenuEntry("Linear, Linear", 4);

    int texture_main_menu = glutCreateMenu(texture_menu_callback);
    glutAddMenuEntry("Toggle Textures", 1);
    glutAddMenuEntry("Grid", 2);
    glutAddMenuEntry("Water", 3);
    glutAddMenuEntry("Bricks", 4);
    glutAddSubMenu("Filters", texture_filter_menu);

    int light_select_menu = glutCreateMenu(light_select_menu_callback);
    glutAddMenuEntry("None", 1);
    glutAddMenuEntry("Point Light", 2);
    glutAddMenuEntry("Spotlight", 3);

    int light_main_menu = glutCreateMenu(light_menu_callback);
    glutAddMenuEntry("Toggle Global Ambient", 1);
    glutAddMenuEntry("Toggle Point Light", 2);
    glutAddMenuEntry("Toggle Directional Light", 3);
    glutAddMenuEntry("Toggle Spotlight", 4);
    glutAddSubMenu("Move Light", light_select_menu);

    glutCreateMenu(menu_callback);
    glutAddSubMenu("Lights", light_main_menu);
    glutAddSubMenu("Textures", texture_main_menu);
    glutAddSubMenu("Floor Material", material_menu);
    glutAddSubMenu("Shading", shading_menu);
    glutAddSubMenu("Interaction Mode", interaction_menu);
    glutAddSubMenu("Animation", animation_menu);
    glutAddMenuEntry("Select Cow", 1);
    glutAddMenuEntry("Select Robot", 2);
    glutAttachMenu(GLUT_RIGHT_BUTTON);
}

void igvInterface::initialize_callbacks() {
    glutKeyboardFunc(keyboardFunc);
    glutSpecialFunc(specialKeyboardFunc);
    glutReshapeFunc(reshapeFunc);
    glutDisplayFunc(displayFunc);
    glutIdleFunc(idleFunc);
    glutMouseFunc(mouseFunc);
    glutMotionFunc(motionFunc);
}

void igvInterface::selectObject(int objectNum) {
    if (objectNum == 1) { // Cow
        selectedObject = triangleMesh;
        currentObject = 1;
    } else if (objectNum == 2) { // Robot
        selectedObject = articulatedModel;
        currentObject = 2;
    }
    selectedLight = -1;
}

void igvInterface::setShading(bool flat) { flatShading = flat; }
void igvInterface::setInteraction(bool keyboard) { articulatedInteractionKeyboard = keyboard; }
void igvInterface::toggleAnimateModel() { animateModel = !animateModel; }
void igvInterface::toggleAnimateCamera() { animateCamera = !animateCamera; }
void igvInterface::toggleAnimateLight() { animateLight = !animateLight; } // Implemented toggle

void igvInterface::setFloorMaterial(int materialIndex) { floor->setMaterial(materialIndex); }
void igvInterface::toggleTexture() { textureEnabled = !textureEnabled; floor->toggleTexture(textureEnabled); }
void igvInterface::setFloorTexture(int textureIndex) { floor->setTexture(textureIndex); }
void igvInterface::setTextureFilter(int filterType) { floor->setTextureFilters(filterType); }

void igvInterface::toggleLight(int lightIndex) {
    if (lightIndex == -1) { // Global ambient
        globalAmbientLightOn = !globalAmbientLightOn;
    } else if (lightIndex >= 0 && lightIndex < lights.size()) {
        lights[lightIndex]->toggle();
    }
}

void igvInterface::selectLight(int lightIndex) {
    selectedLight = lightIndex;
    if (lightIndex != -1) selectedObject = nullptr;
}

void igvInterface::moveSelectedLight(float dx, float dy, float dz) {
    if (selectedLight != -1 && selectedLight < lights.size()) {
        lights[selectedLight]->translate(dx, dy, dz);
    }
}

void menu_callback(int option) {
    igvInterface::getInstance().selectObject(option);
    glutPostRedisplay();
}

void shading_menu_callback(int option) {
    igvInterface::getInstance().setShading(option == 1);
    glutPostRedisplay();
}

void interaction_menu_callback(int option) {
    igvInterface::getInstance().setInteraction(option == 1);
}

void animation_menu_callback(int option) {
    if (option == 1) igvInterface::getInstance().toggleAnimateModel();
    if (option == 2) igvInterface::getInstance().toggleAnimateCamera();
    if (option == 3) igvInterface::getInstance().toggleAnimateLight(); // Added handler for light animation
}

void material_menu_callback(int option) {
    igvInterface::getInstance().setFloorMaterial(option - 1);
    glutPostRedisplay();
}

void texture_menu_callback(int option) {
    switch (option) {
        case 1: igvInterface::getInstance().toggleTexture(); break;
        case 2: igvInterface::getInstance().setFloorTexture(0); break;
        case 3: igvInterface::getInstance().setFloorTexture(1); break;
        case 4: igvInterface::getInstance().setFloorTexture(2); break;
    }
    glutPostRedisplay();
}

void texture_filter_menu_callback(int option) {
    igvInterface::getInstance().setTextureFilter(option - 1);
    glutPostRedisplay();
}

void light_menu_callback(int option) {
    switch (option) {
        case 1: igvInterface::getInstance().toggleLight(-1); break; // Ambient
        case 2: igvInterface::getInstance().toggleLight(0); break;  // Point
        case 3: igvInterface::getInstance().toggleLight(1); break;  // Directional
        case 4: igvInterface::getInstance().toggleLight(2); break;  // Spot
    }
    glutPostRedisplay();
}

void light_select_menu_callback(int option) {
    switch (option) {
        case 1: igvInterface::getInstance().selectLight(-1); break; // None
        case 2: igvInterface::getInstance().selectLight(0); break;  // Point
        case 3: igvInterface::getInstance().selectLight(2); break;  // Spot
    }
    glutPostRedisplay();
}

int igvInterface::get_window_width() { return window_width; }
int igvInterface::get_window_height() { return window_height; }
void igvInterface::set_window_width(int w) { window_width = w; }
void igvInterface::set_window_height(int h) { window_height = h; }

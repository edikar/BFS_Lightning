// GLEW
#define GLEW_STATIC
#include <GL/glew.h>
// GLFW
#include <GLFW/glfw3.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>


void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow *window);
int init();
void showFrameRate();
unsigned int loadTexture(const char *path);
void draw();

#define TRUE (1)
#define FALSE (0)

// settings
#define SCR_WIDTH (1000)
#define SCR_HEIGHT (1000)
#define GRID_SIZE (80)
#define GRID_DENSITY (0.4f)

// timing
float deltaTime = 0.0f;	// time between current frame and last frame
float lastFrame = 0.0f;

typedef struct {
    float x,y,z;
} vec3;

GLFWwindow* window;
int gridProgram, graphProgram;

unsigned int gridVBO, gridVAO, gridEBO;
unsigned int graphVBO, graphWeightVBO, graphVAO, graphEBO;

vec3 gridVertices[GRID_SIZE][GRID_SIZE]; // +1 to include the borders

int gridElements[GRID_SIZE * GRID_SIZE * 2 * 2];
int numGridElements = 0;

struct {
    vec3 vertices[4];
    vec3 gridColor, graphColor;
    int n,s,e,w; //walls
    int visited;
    int fatherX, fatherY; //first father
}graph[GRID_SIZE][GRID_SIZE] = { 0 };

int graphElements[GRID_SIZE * GRID_SIZE * 6] = { 0 };
int numGraphElements = 0;


int finishedFlag = 0;

GLuint LoadShader(GLenum type, const char *shaderSrc){   
    GLuint shader;   
    GLint compiled;   
    // Create the shader object   
    shader = glCreateShader(type);   
    if(shader == 0)      
        return 0;   

    // Load the shader source   
    glShaderSource(shader, 1, &shaderSrc, NULL);   
    // Compile the shader   
    glCompileShader(shader);   
    // Check the compile status   
    glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
    if(!compiled)
    {      
        GLint infoLen = 0;      
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLen);      
    if(infoLen > 1)
    {         
        char* infoLog = malloc(sizeof(char) * infoLen);         
        glGetShaderInfoLog(shader, infoLen, NULL, infoLog);         
        printf("Error compiling shader:\n%s\n", infoLog);         
        free(infoLog);      
    }      
        glDeleteShader(shader);      
        return 0;   
    }   

    return shader;
}



int init_shaders(){  
    GLbyte vShaderStr[] =        
    "#version 330 core           \n"
    "layout (location = 0) in vec3 vPosition;\n"   
    "layout (location = 1) in vec3 vColor;\n"  

    "out vec3 color;        \n"
    "void main()                 \n"      
    "{                           \n"      
    "   gl_Position = vec4(vPosition, 1.0); \n" 
    "   color = vColor; \n"   
    "}                           \n";   

    GLbyte fShaderStr[] =   
    "#version 330 core                          \n"   
    "in vec3 color;        \n"     
    "out vec4 FragColor;                   \n"      
    "uniform vec3 objectColor;"
    "void main()                                \n"      
    "{                                          \n"      
    "  FragColor = vec4( color, 1.0); \n"      
    "}                                          \n";   
    GLuint vertexShader;   
    GLuint fragmentShader;   
    GLuint programObject;   
    GLint linked;
 
// Load the vertex/fragment shaders   
    printf("load vertex shader\n");
    vertexShader = LoadShader(GL_VERTEX_SHADER, vShaderStr);   
    printf("load fragment shader\n");
    fragmentShader = LoadShader(GL_FRAGMENT_SHADER, fShaderStr);   
    // Create the program object   
    programObject = glCreateProgram();   
    if(programObject == 0)      
        return 0;   

    glAttachShader(programObject, vertexShader);   
    glAttachShader(programObject, fragmentShader);    
    // Link the program   
    glLinkProgram(programObject);   
    // Check the link status   
    glGetProgramiv(programObject, GL_LINK_STATUS, &linked);   
    if(!linked)    
    {      
        GLint infoLen = 0;      
        glGetProgramiv(programObject, GL_INFO_LOG_LENGTH, &infoLen);      
        if(infoLen > 1)      
        {         
            char* infoLog = malloc(sizeof(char) * infoLen);         
            glGetProgramInfoLog(programObject, infoLen, NULL, infoLog);         
            printf("Error linking program:\n%s\n", infoLog);         
            free(infoLog);      
        }      
        glDeleteProgram(programObject);      
        return FALSE;   
    }   
    // Store the program object   
    gridProgram = programObject;   
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glLineWidth(3);

    return TRUE;
}

void gridInit()
{
    numGraphElements = 0;
    numGridElements = 0;

    for(int y = 0; y < GRID_SIZE; y++){
        for(int x = 0; x < GRID_SIZE; x++)
        {
            gridVertices[y][x].x = 2 * (GLfloat)x / (GRID_SIZE - 1) - 1.0f;
            gridVertices[y][x].y = 2 * (GLfloat)y / (GRID_SIZE - 1) - 1.0f;
            gridVertices[y][x].z = 0;

        }
    }

    for(int y = 0; y < GRID_SIZE; y++){
        for(int x = 0; x < GRID_SIZE; x++)
        {

            graph[y][x].gridColor.x = 0.8f;
            graph[y][x].gridColor.y = 0.2f;
            graph[y][x].gridColor.z = 0.2f;

            graph[y][x].graphColor.x = 0.8f;
            graph[y][x].graphColor.y = 0.5f;
            graph[y][x].graphColor.z = 0.2f;

            graph[y][x].fatherX = 0;
            graph[y][x].fatherY = 0;

            graph[y][x].visited = 0;
            graph[y][x].n = 0;
            graph[y][x].s = 0;
            graph[y][x].e = 0;
            graph[y][x].w = 0;

            graph[y][x].vertices[0].x = gridVertices[y][x].x;
            graph[y][x].vertices[0].y = gridVertices[y][x].y;
            graph[y][x].vertices[0].z = gridVertices[y][x].z;

            graph[y][x].vertices[1].x = gridVertices[y][x + 1].x;
            graph[y][x].vertices[1].y = gridVertices[y][x + 1].y;
            graph[y][x].vertices[1].z = gridVertices[y][x + 1].z;

            graph[y][x].vertices[2].x = gridVertices[y + 1][x + 1].x;
            graph[y][x].vertices[2].y = gridVertices[y + 1][x + 1].y;
            graph[y][x].vertices[2].z = gridVertices[y + 1][x + 1].z;

            graph[y][x].vertices[3].x = gridVertices[y + 1][x].x;
            graph[y][x].vertices[3].y = gridVertices[y + 1][x].y;
            graph[y][x].vertices[3].z = gridVertices[y + 1][x].z;


        }
    }

    for(int y = 0; y < GRID_SIZE; y++)
    {
        for(int x = 0; x < GRID_SIZE; x++)
        {
            if(( (y == 0) || (y == GRID_SIZE - 1) || (((float)rand() / RAND_MAX) < GRID_DENSITY)) && (x < GRID_SIZE - 1))
            {
                gridElements[numGridElements + 0] = y * GRID_SIZE + x;
                gridElements[numGridElements + 1] = y * GRID_SIZE + x + 1;
                numGridElements += 2;

                graph[y][x].s = 1;
                if(y > 0){
                    graph[y-1][x].n = 1;
                }
            }

            if(( (x == 0) || (x == GRID_SIZE - 1) || (((float)rand() / RAND_MAX) < GRID_DENSITY)) && (y < GRID_SIZE - 1))
            {
                gridElements[numGridElements + 0] = y * GRID_SIZE + x;
                gridElements[numGridElements + 1] = (y + 1) * GRID_SIZE + x;
                numGridElements += 2;

                graph[y][x].e = 1;
                if(x > 0){
                    graph[y][x - 1].w = 1;
                }
            }
        }
    }

    graph[GRID_SIZE - 2][GRID_SIZE / 2].visited = 1;
    graph[GRID_SIZE - 2][GRID_SIZE / 2].graphColor.x = 0.8;
    graph[GRID_SIZE - 2][GRID_SIZE / 2].graphColor.y = 0.5;
    graph[GRID_SIZE - 2][GRID_SIZE / 2].graphColor.z = 0.2;
    for(int y = 0; y < GRID_SIZE; y++)
    {
        for(int x = 0; x < GRID_SIZE; x++)
        {
            if(graph[y][x].visited > 0)
            {
                graphElements[numGraphElements + 0] = y * GRID_SIZE + x;
                graphElements[numGraphElements + 1] = y * GRID_SIZE + x + 1;
                graphElements[numGraphElements + 2] = (y + 1) * GRID_SIZE + x + 1;
                graphElements[numGraphElements + 3] = y * GRID_SIZE + x;
                graphElements[numGraphElements + 4] = (y + 1) * GRID_SIZE + x + 1;
                graphElements[numGraphElements + 5] = (y + 1)  * GRID_SIZE + x; 

                numGraphElements += 6;
            }
        }
    }

    // bind the Vertex Array Object first, then bind and set vertex buffer(s), and then configure vertex attributes(s).
    glBindVertexArray(gridVAO);

    glBindBuffer(GL_ARRAY_BUFFER, gridVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(graph), graph, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gridEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(gridElements), gridElements, GL_STATIC_DRAW);
    //define the position attribute for the shaders
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * 3 * sizeof(float) + 7 * sizeof(int), (void*)0);
    glEnableVertexAttribArray(0); 
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * 3 * sizeof(float) + 7 * sizeof(int), (void*)0 + 4 * 3 * sizeof(float));
    glEnableVertexAttribArray(1); 

    // bind the Vertex Array Object first, then bind and set vertex buffer(s), and then configure vertex attributes(s).
    glBindVertexArray(graphVAO);

    glBindBuffer(GL_ARRAY_BUFFER, graphVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(graph), graph, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * 3 * sizeof(float) + 7 * sizeof(int), (void*)0);
    glEnableVertexAttribArray(0); 
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * 3 * sizeof(float) + 7 * sizeof(int), (void*)0 + 5 * 3 * sizeof(float));
    glEnableVertexAttribArray(1); 

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, graphEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(graphElements), graphElements, GL_DYNAMIC_DRAW);


}

void makeStep(int step)
{
    static int blinking[20] = { 1,0,1,1,0,0,1,0,1,1,0,1,1,1,1,0,1,1,1,1};
    //static int blinking[20] = { 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1 };
    //static int blinking[20] = { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 };

    static int i = 0;
    static saveNumElements = 0;
    if(finishedFlag == 1){
        i++;
        //i = (i >= 20) ? 0 : i + 1;
        if(blinking[i % 20] == 0)
        {
            numGraphElements = 0;
        }else{
            numGraphElements = saveNumElements;
        }

        if(i > 100){
            i = 0;
            finishedFlag = 0;
            gridInit();
            numGraphElements = 0;
        }
        return;
    }


    for(int y = 0; y < GRID_SIZE; y++)
    {
        for(int x = 0; x < GRID_SIZE; x++)
        {
            if(graph[y][x].visited == step)
            {

                if(y == 0) // if visited the bottom layer
                {
                    int tx = x, ty = y;

                    finishedFlag = 1;
                    printf("finished;\n");
                    numGraphElements = 0;

                    graphElements[numGraphElements + 0] = (ty)   * GRID_SIZE + tx;
                    graphElements[numGraphElements + 1] = (ty)   * GRID_SIZE + tx + 1;
                    graphElements[numGraphElements + 2] = (ty+1) * GRID_SIZE + tx + 1;
                    graphElements[numGraphElements + 3] = (ty)   * GRID_SIZE + tx;
                    graphElements[numGraphElements + 4] = (ty+1) * GRID_SIZE + tx + 1;
                    graphElements[numGraphElements + 5] = (ty+1) * GRID_SIZE + tx; 

                    numGraphElements += 6;

                    while(ty < GRID_SIZE - 2)
                    {
                        int ttx = graph[ty][tx].fatherX;
                        ty = graph[ty][tx].fatherY;
                        tx = ttx;

                        graphElements[numGraphElements + 0] = (ty)   * GRID_SIZE + tx;
                        graphElements[numGraphElements + 1] = (ty)   * GRID_SIZE + tx + 1;
                        graphElements[numGraphElements + 2] = (ty+1) * GRID_SIZE + tx + 1;
                        graphElements[numGraphElements + 3] = (ty)   * GRID_SIZE + tx;
                        graphElements[numGraphElements + 4] = (ty+1) * GRID_SIZE + tx + 1;
                        graphElements[numGraphElements + 5] = (ty+1) * GRID_SIZE + tx; 

                        numGraphElements += 6;

                    }
                    saveNumElements = numGraphElements;
                    return;
                }

                printf("graph[%d][%d].visited == %d\n",y, x,step);
                if(graph[y][x].n == 0 && y < GRID_SIZE && graph[y+1][x].visited == 0)
                {
                    printf("graph[%d][%d].n == %d && graph[%d][%d].visited == %d\n", y, x, graph[y][x].n, y+1,x,graph[y+1][x].visited);
                    graphElements[numGraphElements + 0] = (y+1) * GRID_SIZE + x;
                    graphElements[numGraphElements + 1] = (y+1) * GRID_SIZE + x + 1;
                    graphElements[numGraphElements + 2] = (y+2) * GRID_SIZE + x + 1;
                    graphElements[numGraphElements + 3] = (y+1)* GRID_SIZE + x;
                    graphElements[numGraphElements + 4] = (y+2) * GRID_SIZE + x + 1;
                    graphElements[numGraphElements + 5] = (y+2)  * GRID_SIZE + x; 

                    numGraphElements += 6;

                    graph[y+1][x].visited = step + 1;

                    graph[y+1][x].graphColor.x = 0.8;
                    graph[y+1][x].graphColor.y = 0.5;
                    graph[y+1][x].graphColor.z = 0.2;

                    graph[y+1][x].fatherX = x;
                    graph[y+1][x].fatherY = y;
                }
                if(graph[y][x].s == 0 && y > 0 && graph[y-1][x].visited == 0)
                {
                    printf("graph[%d][%d].s == %d && graph[%d][%d].visited == %d\n", y, x, graph[y][x].s, y-1,x,graph[y-1][x].visited);
                    graphElements[numGraphElements + 0] = (y-1) * GRID_SIZE + x;
                    graphElements[numGraphElements + 1] = (y-1) * GRID_SIZE + x + 1;
                    graphElements[numGraphElements + 2] = (y) * GRID_SIZE + x + 1;
                    graphElements[numGraphElements + 3] = (y-1)* GRID_SIZE + x;
                    graphElements[numGraphElements + 4] = (y) * GRID_SIZE + x + 1;
                    graphElements[numGraphElements + 5] = (y)  * GRID_SIZE + x; 

                    numGraphElements += 6;

                    graph[y-1][x].visited = step + 1;
                    graph[y-1][x].graphColor.x = 0.8;
                    graph[y-1][x].graphColor.y = 0.5;
                    graph[y-1][x].graphColor.z = 0.2;

                    graph[y-1][x].fatherX = x;
                    graph[y-1][x].fatherY = y;
                }
                if(graph[y][x].e == 0 && x > 0 && graph[y][x-1].visited == 0)
                {
                    printf("graph[%d][%d].e == %d && graph[%d][%d].visited == %d\n", y, x, graph[y][x].e, y,x-1,graph[y][x-1].visited);
                    graphElements[numGraphElements + 0] = (y) * GRID_SIZE + x-1;
                    graphElements[numGraphElements + 1] = (y) * GRID_SIZE + x;
                    graphElements[numGraphElements + 2] = (y+1) * GRID_SIZE + x;
                    graphElements[numGraphElements + 3] = (y)* GRID_SIZE + x-1;
                    graphElements[numGraphElements + 4] = (y+1) * GRID_SIZE + x;
                    graphElements[numGraphElements + 5] = (y+1)  * GRID_SIZE + x-1; 

                    numGraphElements += 6;

                    graph[y][x-1].visited = step + 1;
                    graph[y][x-1].graphColor.x = 0.8;
                    graph[y][x-1].graphColor.y = 0.5;
                    graph[y][x-1].graphColor.z = 0.2;
                    
                    graph[y][x-1].fatherX = x;
                    graph[y][x-1].fatherY = y;
                }
                if(graph[y][x].w == 0 && x < GRID_SIZE && graph[y][x+1].visited == 0)
                {
                    printf("graph[%d][%d].w == %d && graph[%d][%d].visited == %d\n", y, x, graph[y][x].e, y,x+1,graph[y][x+1].visited);
                    graphElements[numGraphElements + 0] = (y) * GRID_SIZE + x+1;
                    graphElements[numGraphElements + 1] = (y) * GRID_SIZE + x+2;
                    graphElements[numGraphElements + 2] = (y+1) * GRID_SIZE + x+2;
                    graphElements[numGraphElements + 3] = (y)* GRID_SIZE + x+1;
                    graphElements[numGraphElements + 4] = (y+1) * GRID_SIZE + x+2;
                    graphElements[numGraphElements + 5] = (y+1)  * GRID_SIZE + x+1; 

                    numGraphElements += 6;

                    graph[y][x+1].visited = step + 1;
                    graph[y][x+1].graphColor.x = 0.8;
                    graph[y][x+1].graphColor.y = 0.5;
                    graph[y][x+1].graphColor.z = 0.2;
                    
                    graph[y][x+1].fatherX = x;
                    graph[y][x+1].fatherY = y;
                }
            }
        }
    }
}

void draw(){  
    // Set the viewport   
    glViewport(0, 0, SCR_HEIGHT, SCR_WIDTH);   
    // Clear  
    glClearColor(0.1f, 0.1f, 0.2f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    // Use the program object   
    glUseProgram(gridProgram);   

    //filled or wireframe triangles
    //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    glBindVertexArray(gridVAO);
    float gridColor[3] = { 0.9f, 0.2f, 0.2f };
    glUniform3fv(glGetUniformLocation(gridProgram, "objectColor"), 1, gridColor);
    glDrawElements(GL_LINES, numGridElements, GL_UNSIGNED_INT, 0);


    glBindVertexArray(graphVAO);
    float graphColor[3] = { 0.8f, 0.5f, 0.2f };
    glUniform3fv(glGetUniformLocation(gridProgram, "objectColor"), 1, graphColor);
    glDrawElements(GL_TRIANGLES, numGraphElements, GL_UNSIGNED_INT, 0);

    static int step = 0;
    if(step == 0) //skip on first frame to make sure it is always drawn
        step++;
    else {
        makeStep(step++);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, graphEBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(graphElements), graphElements, GL_DYNAMIC_DRAW);
    }
}


int main()
{
	GLint status = 0;

	//init project
	status = init();	
	if(status != 0)
	{
		printf("init() failed at  %s:%d (%s)", __FILE__ , __LINE__ , __FUNCTION__);
		return 1;
	}

    init_shaders();

    // render loop
    // -----------
    while (!glfwWindowShouldClose(window))
    {
        // per-frame time logic
        // --------------------
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // input
        // -----
        processInput(window);

        draw();

        // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
        // -------------------------------------------------------------------------------
        glfwSwapBuffers(window);
        glfwPollEvents();

    }

    // glfw: terminate, clearing all previously allocated GLFW resources.
    // ------------------------------------------------------------------
    glfwTerminate();
    return 0;
}



//Init glfw, create and init window, init glew
int init()
{
    srand(time(0));
    
    // glfw: initialize and configure
    // ------------------------------
	glfwInit();
	//the following 3 lines define opengl version 4.5 and the core profile
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_RESIZABLE, GL_TRUE);

	window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Lightning", 0, 0);
	if (window == 0)
	{
		printf("Failed to create GLFW window\n");
		glfwTerminate();
		return 1;
	}
	
	glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    //glfwSetCursorPosCallback(window, mouse_callback);
    //glfwSetScrollCallback(window, scroll_callback); 

	//glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);  

	glewExperimental = GL_TRUE;
	if (glewInit() != GLEW_OK)
	{
		printf("Failed to initialize GLEW\n");
		return 1;
	}

	glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);

    glGenVertexArrays(1, &gridVAO);
    glGenBuffers(1, &gridVBO);
    glGenBuffers(1, &gridEBO);



    glGenVertexArrays(1, &graphVAO);
    glGenBuffers(1, &graphVBO);
    glGenBuffers(1, &graphWeightVBO);
    glGenBuffers(1, &graphEBO);


    gridInit();


	return 0;
}


// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
void processInput(GLFWwindow *window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, TRUE);
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    // make sure the viewport matches the new window dimensions; note that width and 
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
}

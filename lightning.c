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

#define GRID_R (0.5f)
#define GRID_G (0.5f)
#define GRID_B (0.5f)

#define GRAPH_R (0.5f)
#define GRAPH_G (0.5f)
#define GRAPH_B (0.5f)

#define TRUE (1)
#define FALSE (0)

// settings
#define SCR_WIDTH (1000)
#define SCR_HEIGHT (1000)
#define GRID_SIZE (81) /* 300 */
#define GRID_DENSITY (0.4f)

// timing
float deltaTime = 0.0f;	// time between current frame and last frame
float lastFrame = 0.0f;

typedef struct {
    float x,y,z;
} vec3;

GLFWwindow* window;
int gridProgram, graphProgram;

unsigned int gridVBO, gridColorsVBO, gridVAO, gridEBO;
unsigned int graphVBO, graphColorsVBO, graphVAO, graphEBO;

vec3 gridVertices[GRID_SIZE][GRID_SIZE]; // +1 to include the borders
vec3 gridColors[GRID_SIZE][GRID_SIZE] = { 0 };
int gridElements[GRID_SIZE * GRID_SIZE * 2 * 2];
int numGridElements = 0;


vec3 graphVertices[GRID_SIZE][GRID_SIZE][4] = { 0 };
vec3 graphColors[GRID_SIZE][GRID_SIZE][4] = { 0 };
struct {
    int n,s,e,w; //walls
    int visited;
    int fatherX, fatherY; //first father
}graphProperties[GRID_SIZE][GRID_SIZE];


int graphElements[GRID_SIZE * GRID_SIZE * 6] = { 0 };
int numGraphElements = 0;


int step = 0;
int finishedFlag = 0;
float brightnessFactor = 1.0;

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
    "uniform float brightnessFactor;"
    "void main()                                \n"      
    "{                                          \n"      
    "  FragColor = vec4(brightnessFactor * color, 1.0); \n"      
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
    glLineWidth(1);

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

            gridColors[y][x].x = GRID_R;
            gridColors[y][x].y = GRID_G;
            gridColors[y][x].z = GRID_B;

            for(int i = 0; i < 4; i++){
                graphColors[y][x][i].x = GRAPH_R;
                graphColors[y][x][i].y = GRAPH_G;
                graphColors[y][x][i].z = GRAPH_B;
            }
            graphProperties[y][x].fatherX = 0;
            graphProperties[y][x].fatherY = 0;

            graphProperties[y][x].visited = 0;
            graphProperties[y][x].n = 0;
            graphProperties[y][x].s = 0;
            graphProperties[y][x].e = 0;
            graphProperties[y][x].w = 0;

            graphVertices[y][x][0].x = gridVertices[y][x].x;
            graphVertices[y][x][0].y = gridVertices[y][x].y;
            graphVertices[y][x][0].z = gridVertices[y][x].z;

            graphVertices[y][x][1].x = gridVertices[y][x + 1].x;
            graphVertices[y][x][1].y = gridVertices[y][x + 1].y;
            graphVertices[y][x][1].z = gridVertices[y][x + 1].z;

            graphVertices[y][x][2].x = gridVertices[y + 1][x + 1].x;
            graphVertices[y][x][2].y = gridVertices[y + 1][x + 1].y;
            graphVertices[y][x][2].z = gridVertices[y + 1][x + 1].z;

            graphVertices[y][x][3].x = gridVertices[y + 1][x].x;
            graphVertices[y][x][3].y = gridVertices[y + 1][x].y;
            graphVertices[y][x][3].z = gridVertices[y + 1][x].z;


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

                graphProperties[y][x].s = 1;
                if(y > 0){
                    graphProperties[y-1][x].n = 1;
                }
            }

            if(( (x == 0) || (x == GRID_SIZE - 1) || (((float)rand() / RAND_MAX) < GRID_DENSITY)) && (y < GRID_SIZE - 1))
            {
                gridElements[numGridElements + 0] = y * GRID_SIZE + x;
                gridElements[numGridElements + 1] = (y + 1) * GRID_SIZE + x;
                numGridElements += 2;

                graphProperties[y][x].e = 1;
                if(x > 0){
                    graphProperties[y][x - 1].w = 1;
                }
            }
        }
    }

    graphProperties[GRID_SIZE - 2][GRID_SIZE / 2].visited = 1;
    for(int i = 0; i < 4; i++){
        graphColors[GRID_SIZE - 2][GRID_SIZE / 2][i].x = GRAPH_R;
        graphColors[GRID_SIZE - 2][GRID_SIZE / 2][i].y = GRAPH_G;
        graphColors[GRID_SIZE - 2][GRID_SIZE / 2][i].z = GRAPH_B;
    }
    for(int y = 0; y < GRID_SIZE; y++)
    {
        for(int x = 0; x < GRID_SIZE; x++)
        {
            if(graphProperties[y][x].visited > 0)
            {
                graphElements[numGraphElements + 0] = 4*(y * GRID_SIZE + x);
                graphElements[numGraphElements + 1] = 4*(y * GRID_SIZE + x) + 1;
                graphElements[numGraphElements + 2] = 4*(y * GRID_SIZE + x) + 2;
                graphElements[numGraphElements + 3] = 4*(y * GRID_SIZE + x);
                graphElements[numGraphElements + 4] = 4*(y * GRID_SIZE + x) + 2;
                graphElements[numGraphElements + 5] = 4*(y * GRID_SIZE + x) + 3;

                numGraphElements += 6;
            }
        }
    }

    // bind the Vertex Array Object first, then bind and set vertex buffer(s), and then configure vertex attributes(s).
    glBindVertexArray(gridVAO);

    glBindBuffer(GL_ARRAY_BUFFER, gridVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(gridVertices), gridVertices, GL_STATIC_DRAW);
    //define the position attribute for the shaders
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0); 
    glBindBuffer(GL_ARRAY_BUFFER, gridVBO);
    glBindBuffer(GL_ARRAY_BUFFER, gridColorsVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(gridColors), gridColors, GL_STATIC_DRAW);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1); 
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gridEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(gridElements), gridElements, GL_STATIC_DRAW);

    // bind the Vertex Array Object first, then bind and set vertex buffer(s), and then configure vertex attributes(s).
    glBindVertexArray(graphVAO);


    glBindBuffer(GL_ARRAY_BUFFER, graphVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(graphVertices), graphVertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0); 
    glBindBuffer(GL_ARRAY_BUFFER, graphColorsVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(graphColors), graphColors, GL_DYNAMIC_DRAW);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 4 * 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1); 


    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, graphEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, numGraphElements * sizeof(int), graphElements, GL_DYNAMIC_DRAW);


}

void makeStep()
{
    int emptyStepFlag = 1;

    static int blinking[40] = { 1,0,1,1,0,0,1,0,1,1,0,1,1,1,1,0,1,1,1,1,1,0,0,0,0,0,1,0,1,1,0,1,1,1,1,1,1,1,1,1};
    //static int blinking[20] = { 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1 };
    //static int blinking[20] = { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 };

    static int i = 0;
    static saveNumElements = 0;
    if(finishedFlag == 1){
        i++;
        //i = (i >= 20) ? 0 : i + 1;
        if(((float)rand() / RAND_MAX) < 0.7f)
        {
            brightnessFactor *= 0.8;
            glUniform1f(glGetUniformLocation(gridProgram, "brightnessFactor"), brightnessFactor);
            //numGraphElements = 0;
        }else{
            brightnessFactor = 1.0;
            glUniform1f(glGetUniformLocation(gridProgram, "brightnessFactor"), brightnessFactor);
            //numGraphElements = saveNumElements;
        }

        if(i > 100){
            i = 0;
            finishedFlag = 0;
            step = 0;
            brightnessFactor = 1.0;
            gridInit();
            numGraphElements = 0;
        }
        return;
    }

    //dim previous steps
    for(int y = 0; y < GRID_SIZE; y++)
    {
        for(int x = 0; x < GRID_SIZE; x++)
        {
            if(graphProperties[y][x].visited > 0)
            {
                for(int i = 0; i < 4; i++)
                {
                    graphColors[y][x][i].x *= 0.9;
                    graphColors[y][x][i].y *= 0.9;
                    graphColors[y][x][i].z *= 0.9;
                }
            }
        }
    }

    for(int y = 0; y < GRID_SIZE; y++)
    {
        for(int x = 0; x < GRID_SIZE; x++)
        {
            if(graphProperties[y][x].visited == step)
            {
                if(y == 0) // if visited the bottom layer
                {
                    int tx = x, ty = y;

                    finishedFlag = 1;
                    printf("finished;\n");
                    numGraphElements = 0;


                    while(ty < GRID_SIZE - 2)
                    {
                        graphElements[numGraphElements + 0] = 4*((ty)   * GRID_SIZE + tx);
                        graphElements[numGraphElements + 1] = 4*((ty)   * GRID_SIZE + tx) + 1;
                        graphElements[numGraphElements + 2] = 4*((ty)   * GRID_SIZE + tx) + 2;
                        graphElements[numGraphElements + 3] = 4*((ty)   * GRID_SIZE + tx);
                        graphElements[numGraphElements + 4] = 4*((ty)   * GRID_SIZE + tx) + 2;
                        graphElements[numGraphElements + 5] = 4*((ty)   * GRID_SIZE + tx) + 3;

                        numGraphElements += 6;

                        for(int i = 0; i < 4; i++){
                            graphColors[ty][tx][i].x = 1.0f;
                            graphColors[ty][tx][i].y = 1.0f;
                            graphColors[ty][tx][i].z = 1.0f;
                        }

                        int ttx = graphProperties[ty][tx].fatherX;
                        ty = graphProperties[ty][tx].fatherY;
                        tx = ttx;
                    }
                    saveNumElements = numGraphElements;
                    return;
                }

                //printf("graph[%d][%d].visited == %d\n",y, x,step);
                if(graphProperties[y][x].n == 0 && y < GRID_SIZE && graphProperties[y+1][x].visited == 0)
                {
                    emptyStepFlag = 0;
                    //printf("graph[%d][%d].n == %d && graph[%d][%d].visited == %d\n", y, x, graphProperties[y][x].n, y+1,x,graphProperties[y+1][x].visited);
                    graphElements[numGraphElements + 0] = 4*((y+1) * GRID_SIZE + x);
                    graphElements[numGraphElements + 1] = 4*((y+1) * GRID_SIZE + x) + 1;
                    graphElements[numGraphElements + 2] = 4*((y+1) * GRID_SIZE + x) + 2;
                    graphElements[numGraphElements + 3] = 4*((y+1) * GRID_SIZE + x);
                    graphElements[numGraphElements + 4] = 4*((y+1) * GRID_SIZE + x) + 2;
                    graphElements[numGraphElements + 5] = 4*((y+1) * GRID_SIZE + x) + 3;

                    numGraphElements += 6;

                    graphProperties[y+1][x].visited = step + 1;

                    for(int i = 0; i < 4; i++){
                        graphColors[y+1][x][i].x = GRAPH_R;
                        graphColors[y+1][x][i].y = GRAPH_G;
                        graphColors[y+1][x][i].z = GRAPH_B;
                    }

                    graphProperties[y+1][x].fatherX = x;
                    graphProperties[y+1][x].fatherY = y;
                }
                if(graphProperties[y][x].s == 0 && y > 0 && graphProperties[y-1][x].visited == 0)
                {
                    emptyStepFlag = 0;
                    //printf("graph[%d][%d].s == %d && graph[%d][%d].visited == %d\n", y, x, graphProperties[y][x].s, y-1,x,graphProperties[y-1][x].visited);
                    graphElements[numGraphElements + 0] = 4*((y-1) * GRID_SIZE + x);
                    graphElements[numGraphElements + 1] = 4*((y-1) * GRID_SIZE + x) + 1;
                    graphElements[numGraphElements + 2] = 4*((y-1) * GRID_SIZE + x) + 2;
                    graphElements[numGraphElements + 3] = 4*((y-1) * GRID_SIZE + x);
                    graphElements[numGraphElements + 4] = 4*((y-1) * GRID_SIZE + x) + 2;
                    graphElements[numGraphElements + 5] = 4*((y-1) * GRID_SIZE + x) + 3;

                    numGraphElements += 6;

                    graphProperties[y-1][x].visited = step + 1;

                    for(int i = 0; i < 4; i++){
                        graphColors[y-1][x][i].x = GRAPH_R;
                        graphColors[y-1][x][i].y = GRAPH_G;
                        graphColors[y-1][x][i].z = GRAPH_B;
                    }

                    graphProperties[y-1][x].fatherX = x;
                    graphProperties[y-1][x].fatherY = y;
                }
                if(graphProperties[y][x].e == 0 && x > 0 && graphProperties[y][x-1].visited == 0)
                {
                    emptyStepFlag = 0;
                    //printf("graph[%d][%d].e == %d && graph[%d][%d].visited == %d\n", y, x, graphProperties[y][x].e, y,x-1,graphProperties[y][x-1].visited);
                    graphElements[numGraphElements + 0] = 4*((y) * GRID_SIZE + x-1);
                    graphElements[numGraphElements + 1] = 4*((y) * GRID_SIZE + x-1) + 1;
                    graphElements[numGraphElements + 2] = 4*((y) * GRID_SIZE + x-1) + 2;
                    graphElements[numGraphElements + 3] = 4*((y) * GRID_SIZE + x-1);
                    graphElements[numGraphElements + 4] = 4*((y) * GRID_SIZE + x-1) + 2;
                    graphElements[numGraphElements + 5] = 4*((y) * GRID_SIZE + x-1) + 3;

                    numGraphElements += 6;

                    graphProperties[y][x-1].visited = step + 1;

                    for(int i = 0; i < 4; i++){
                        graphColors[y][x-1][i].x = GRAPH_R;
                        graphColors[y][x-1][i].y = GRAPH_G;
                        graphColors[y][x-1][i].z = GRAPH_B;
                    }
                    
                    graphProperties[y][x-1].fatherX = x;
                    graphProperties[y][x-1].fatherY = y;
                }
                if(graphProperties[y][x].w == 0 && x < GRID_SIZE && graphProperties[y][x+1].visited == 0)
                {
                    emptyStepFlag = 0;
                    //printf("graph[%d][%d].w == %d && graph[%d][%d].visited == %d\n", y, x, graphProperties[y][x].e, y,x+1,graphProperties[y][x+1].visited);
                    graphElements[numGraphElements + 0] = 4*((y) * GRID_SIZE + x+1);
                    graphElements[numGraphElements + 1] = 4*((y) * GRID_SIZE + x+1) + 1;
                    graphElements[numGraphElements + 2] = 4*((y) * GRID_SIZE + x+1) + 2;
                    graphElements[numGraphElements + 3] = 4*((y) * GRID_SIZE + x+1);
                    graphElements[numGraphElements + 4] = 4*((y) * GRID_SIZE + x+1) + 2;
                    graphElements[numGraphElements + 5] = 4*((y) * GRID_SIZE + x+1) + 3;

                    numGraphElements += 6;

                    graphProperties[y][x+1].visited = step + 1;

                    for(int i = 0; i < 4; i++){
                        graphColors[y][x+1][i].x = GRAPH_R;
                        graphColors[y][x+1][i].y = GRAPH_G;
                        graphColors[y][x+1][i].z = GRAPH_B;
                    }
                    graphProperties[y][x+1].fatherX = x;
                    graphProperties[y][x+1].fatherY = y;
                }

            }
        }
    }

    if(emptyStepFlag == 1){
        finishedFlag = 0;
        step = 0;
        gridInit();
    }
}

void draw(){  
    // Set the viewport   
    glViewport(0, 0, SCR_HEIGHT, SCR_WIDTH);   
    // Clear  
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    // Use the program object   
    glUseProgram(gridProgram);   

    //filled or wireframe triangles
    //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    glBindVertexArray(gridVAO);
    glUniform1f(glGetUniformLocation(gridProgram, "brightnessFactor"), 1.0);

    glDrawElements(GL_LINES, numGridElements, GL_UNSIGNED_INT, 0);

    glBindVertexArray(graphVAO);
    glUniform1f(glGetUniformLocation(gridProgram, "brightnessFactor"), brightnessFactor);

    glDrawElements(GL_TRIANGLES, numGraphElements, GL_UNSIGNED_INT, 0);

    if(step == 0) //skip on first frame to make sure it is always drawn
        step++;
    else {
        makeStep();
        step++;

        glBindBuffer(GL_ARRAY_BUFFER, graphColorsVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(graphColors), graphColors, GL_DYNAMIC_DRAW);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE,  3 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1); 

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, graphEBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, numGraphElements * sizeof(int), graphElements, GL_DYNAMIC_DRAW);

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
    glGenBuffers(1, &gridColorsVBO);
    glGenBuffers(1, &gridEBO);



    glGenVertexArrays(1, &graphVAO);
    glGenBuffers(1, &graphVBO);
    glGenBuffers(1, &graphColorsVBO);
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

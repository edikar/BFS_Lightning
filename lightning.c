// GLEW
#define GLEW_STATIC
#include <GL/glew.h>
// GLFW
#include <GLFW/glfw3.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <string.h>


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
static int gSCR_WIDTH = 1000;
static int gSCR_HEIGHT = 1000;
static float gGRID_DENSITY = 0.4f;
static float gDIM_FACTOR = 0.95f;

#ifndef gGRID_SIZE
#define gGRID_SIZE (80) /*(81) /* 300 */
#endif

typedef struct {
    float x,y,z;
} vec3;

GLFWwindow* window;
int gridProgram, graphProgram;

unsigned int gridVBO, gridColorsVBO, gridVAO, gridEBO;
unsigned int graphVBO, graphColorsVBO, graphVAO, graphEBO;

vec3 gridVertices[gGRID_SIZE][gGRID_SIZE]; // +1 to include the borders
vec3 gridColors[gGRID_SIZE][gGRID_SIZE] = { 0 };
int gridElements[gGRID_SIZE * gGRID_SIZE * 2 * 2];
int numGridElements = 0;


vec3 graphVertices[gGRID_SIZE][gGRID_SIZE][4] = { 0 };
struct {
    int x,y;
}graphVisited[gGRID_SIZE * gGRID_SIZE] = { 0 };
int visitedNextInQ = 0, visitedNum = 0;
vec3 graphColors[gGRID_SIZE][gGRID_SIZE][4] = { 0 };
struct {
    int n,s,e,w; //walls
    int visited;
    int fatherX, fatherY; //first father
}graphProperties[gGRID_SIZE][gGRID_SIZE];


int graphElements[gGRID_SIZE * gGRID_SIZE * 6] = { 0 };
int numGraphElements = 0;


int step = 0;
int finishedFlag = 0;
int loopFlag = 0;
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

    memset(graphVisited, -1, sizeof(graphVisited));
    visitedNum = 0;
    visitedNextInQ = 0;

    for(int y = 0; y < gGRID_SIZE; y++){
        for(int x = 0; x < gGRID_SIZE; x++)
        {
            gridVertices[y][x].x = 2 * (GLfloat)x / (gGRID_SIZE - 1) - 1.0f;
            gridVertices[y][x].y = 2 * (GLfloat)y / (gGRID_SIZE - 1) - 1.0f;
            gridVertices[y][x].z = 0;

        }
    }

    for(int y = 0; y < gGRID_SIZE - 1; y++){
        for(int x = 0; x < gGRID_SIZE - 1; x++)
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

    for(int y = 0; y < gGRID_SIZE; y++)
    {
        for(int x = 0; x < gGRID_SIZE; x++)
        {
            if(( (y == 0) || (y == gGRID_SIZE - 1) || (((float)rand() / RAND_MAX) < gGRID_DENSITY)) && (x < gGRID_SIZE - 1))
            {
                gridElements[numGridElements + 0] = y * gGRID_SIZE + x;
                gridElements[numGridElements + 1] = y * gGRID_SIZE + x + 1;
                numGridElements += 2;

                graphProperties[y][x].s = 1;
                if(y > 0){
                    graphProperties[y-1][x].n = 1;
                }
            }

            if(( (x == 0) || (x == gGRID_SIZE - 1) || (((float)rand() / RAND_MAX) < gGRID_DENSITY)) && (y < gGRID_SIZE - 1))
            {
                gridElements[numGridElements + 0] = y * gGRID_SIZE + x;
                gridElements[numGridElements + 1] = (y + 1) * gGRID_SIZE + x;
                numGridElements += 2;

                graphProperties[y][x].e = 1;
                if(x > 0){
                    graphProperties[y][x - 1].w = 1;
                }
            }
        }
    }

    int startY = gGRID_SIZE - 2;
    int startX = gGRID_SIZE / 2;
    graphProperties[startY][startX].visited = 1;
    graphProperties[startY][startX].fatherX = gGRID_SIZE;
    graphProperties[startY][startX].fatherY = gGRID_SIZE;
    graphVisited[visitedNum].x = startX;
    graphVisited[visitedNum].y = startY;
    visitedNum++;

    for(int i = 0; i < 4; i++){
        graphColors[startY][startX][i].x = GRAPH_R;
        graphColors[startY][startX][i].y = GRAPH_G;
        graphColors[startY][startX][i].z = GRAPH_B;
    }

    graphElements[numGraphElements + 0] = 4*(startY * gGRID_SIZE + startX);
    graphElements[numGraphElements + 1] = 4*(startY * gGRID_SIZE + startX) + 1;
    graphElements[numGraphElements + 2] = 4*(startY * gGRID_SIZE + startX) + 2;
    graphElements[numGraphElements + 3] = 4*(startY * gGRID_SIZE + startX);
    graphElements[numGraphElements + 4] = 4*(startY * gGRID_SIZE + startX) + 2;
    graphElements[numGraphElements + 5] = 4*(startY * gGRID_SIZE + startX) + 3;

    numGraphElements += 6;


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
    int emptyStepFlag = 1; //If no change in current step - then the maze is unsolvable

    static int blinking = 0;
    if(finishedFlag == 1){
        blinking++;
        if(((float)rand() / RAND_MAX) < 0.85f)
        {
            brightnessFactor *= 0.9;
            glUniform1f(glGetUniformLocation(gridProgram, "brightnessFactor"), brightnessFactor);
        }else{
            brightnessFactor = 1.0;
            glUniform1f(glGetUniformLocation(gridProgram, "brightnessFactor"), brightnessFactor);
        }

        if(blinking > 100){
            if(loopFlag)
            {
                blinking = 0;
                finishedFlag = 0;
                step = 0;
                brightnessFactor = 1.0;
                gridInit();
                numGraphElements = 0;
            }else{
                exit(0);
            }
        }
        return;
    }

    //dim previous steps
    for(int v = 0; v < visitedNum; v++)
    {
        int x = graphVisited[v].x;
        int y = graphVisited[v].y;

        for(int i = 0; i < 4; i++)
        {
            graphColors[y][x][i].x *= gDIM_FACTOR;
            graphColors[y][x][i].y *= gDIM_FACTOR;
            graphColors[y][x][i].z *= gDIM_FACTOR;
        }
    }

    static int s = 0;
    //printf("step = %d, visitedNextInQ = %d, visitedNum = %d\n", s++, visitedNextInQ, visitedNum);
    int lastVisitedSize = visitedNum; //we are going to updated visitedNum inside the loop so save it in temporary variable
    for(int v = visitedNextInQ; v < lastVisitedSize; v++)
    {
        int x = graphVisited[v].x;
        int y = graphVisited[v].y;
        //printf("%d: x,y = %d,%d\n",v, x,y);

        if(y == 0) // if visited the bottom layer - make the lightning strike
        {
            int tx = x, ty = y;

            finishedFlag = 1;
            //printf("finished;\n");
            numGraphElements = 0;

            while(ty < gGRID_SIZE - 1)
            {
                graphElements[numGraphElements + 0] = 4*((ty)   * gGRID_SIZE + tx);
                graphElements[numGraphElements + 1] = 4*((ty)   * gGRID_SIZE + tx) + 1;
                graphElements[numGraphElements + 2] = 4*((ty)   * gGRID_SIZE + tx) + 2;
                graphElements[numGraphElements + 3] = 4*((ty)   * gGRID_SIZE + tx);
                graphElements[numGraphElements + 4] = 4*((ty)   * gGRID_SIZE + tx) + 2;
                graphElements[numGraphElements + 5] = 4*((ty)   * gGRID_SIZE + tx) + 3;
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
            return;
        }

        //printf("graph[%d][%d].visited == %d\n",y, x,step);
        if(graphProperties[y][x].n == 0 && y < gGRID_SIZE && graphProperties[y+1][x].visited == 0)
        {
            emptyStepFlag = 0;
            //printf("graph[%d][%d].n == %d && graph[%d][%d].visited == %d\n", y, x, graphProperties[y][x].n, y+1,x,graphProperties[y+1][x].visited);
            graphElements[numGraphElements + 0] = 4*((y+1) * gGRID_SIZE + x);
            graphElements[numGraphElements + 1] = 4*((y+1) * gGRID_SIZE + x) + 1;
            graphElements[numGraphElements + 2] = 4*((y+1) * gGRID_SIZE + x) + 2;
            graphElements[numGraphElements + 3] = 4*((y+1) * gGRID_SIZE + x);
            graphElements[numGraphElements + 4] = 4*((y+1) * gGRID_SIZE + x) + 2;
            graphElements[numGraphElements + 5] = 4*((y+1) * gGRID_SIZE + x) + 3;

            numGraphElements += 6;

            graphProperties[y+1][x].visited = step + 1;

            for(int i = 0; i < 4; i++){
                graphColors[y+1][x][i].x = GRAPH_R;
                graphColors[y+1][x][i].y = GRAPH_G;
                graphColors[y+1][x][i].z = GRAPH_B;
            }

            graphProperties[y+1][x].fatherX = x;
            graphProperties[y+1][x].fatherY = y;

            graphVisited[visitedNum].x = x;
            graphVisited[visitedNum].y = y+1;
            visitedNum++;

        }
        if(graphProperties[y][x].s == 0 && y > 0 && graphProperties[y-1][x].visited == 0)
        {
            emptyStepFlag = 0;
            //printf("graph[%d][%d].s == %d && graph[%d][%d].visited == %d\n", y, x, graphProperties[y][x].s, y-1,x,graphProperties[y-1][x].visited);
            graphElements[numGraphElements + 0] = 4*((y-1) * gGRID_SIZE + x);
            graphElements[numGraphElements + 1] = 4*((y-1) * gGRID_SIZE + x) + 1;
            graphElements[numGraphElements + 2] = 4*((y-1) * gGRID_SIZE + x) + 2;
            graphElements[numGraphElements + 3] = 4*((y-1) * gGRID_SIZE + x);
            graphElements[numGraphElements + 4] = 4*((y-1) * gGRID_SIZE + x) + 2;
            graphElements[numGraphElements + 5] = 4*((y-1) * gGRID_SIZE + x) + 3;

            numGraphElements += 6;

            graphProperties[y-1][x].visited = step + 1;

            for(int i = 0; i < 4; i++){
                graphColors[y-1][x][i].x = GRAPH_R;
                graphColors[y-1][x][i].y = GRAPH_G;
                graphColors[y-1][x][i].z = GRAPH_B;
            }

            graphProperties[y-1][x].fatherX = x;
            graphProperties[y-1][x].fatherY = y;

            graphVisited[visitedNum].x = x;
            graphVisited[visitedNum].y = y-1;
            visitedNum++;
        }
        if(graphProperties[y][x].e == 0 && x > 0 && graphProperties[y][x-1].visited == 0)
        {
            emptyStepFlag = 0;
            //printf("graph[%d][%d].e == %d && graph[%d][%d].visited == %d\n", y, x, graphProperties[y][x].e, y,x-1,graphProperties[y][x-1].visited);
            graphElements[numGraphElements + 0] = 4*((y) * gGRID_SIZE + x-1);
            graphElements[numGraphElements + 1] = 4*((y) * gGRID_SIZE + x-1) + 1;
            graphElements[numGraphElements + 2] = 4*((y) * gGRID_SIZE + x-1) + 2;
            graphElements[numGraphElements + 3] = 4*((y) * gGRID_SIZE + x-1);
            graphElements[numGraphElements + 4] = 4*((y) * gGRID_SIZE + x-1) + 2;
            graphElements[numGraphElements + 5] = 4*((y) * gGRID_SIZE + x-1) + 3;

            numGraphElements += 6;

            graphProperties[y][x-1].visited = step + 1;

            for(int i = 0; i < 4; i++){
                graphColors[y][x-1][i].x = GRAPH_R;
                graphColors[y][x-1][i].y = GRAPH_G;
                graphColors[y][x-1][i].z = GRAPH_B;
            }
            
            graphProperties[y][x-1].fatherX = x;
            graphProperties[y][x-1].fatherY = y;

            graphVisited[visitedNum].x = x-1;
            graphVisited[visitedNum].y = y;
            visitedNum++;
        }
        if(graphProperties[y][x].w == 0 && x < gGRID_SIZE && graphProperties[y][x+1].visited == 0)
        {
            emptyStepFlag = 0;
            //printf("graph[%d][%d].w == %d && graph[%d][%d].visited == %d\n", y, x, graphProperties[y][x].e, y,x+1,graphProperties[y][x+1].visited);
            graphElements[numGraphElements + 0] = 4*((y) * gGRID_SIZE + x+1);
            graphElements[numGraphElements + 1] = 4*((y) * gGRID_SIZE + x+1) + 1;
            graphElements[numGraphElements + 2] = 4*((y) * gGRID_SIZE + x+1) + 2;
            graphElements[numGraphElements + 3] = 4*((y) * gGRID_SIZE + x+1);
            graphElements[numGraphElements + 4] = 4*((y) * gGRID_SIZE + x+1) + 2;
            graphElements[numGraphElements + 5] = 4*((y) * gGRID_SIZE + x+1) + 3;

            numGraphElements += 6;

            graphProperties[y][x+1].visited = step + 1;

            for(int i = 0; i < 4; i++){
                graphColors[y][x+1][i].x = GRAPH_R;
                graphColors[y][x+1][i].y = GRAPH_G;
                graphColors[y][x+1][i].z = GRAPH_B;
            }
            graphProperties[y][x+1].fatherX = x;
            graphProperties[y][x+1].fatherY = y;

            graphVisited[visitedNum].x = x+1;
            graphVisited[visitedNum].y = y;
            visitedNum++;
        }
    }

    visitedNextInQ = lastVisitedSize; //update next in visited Q


    if(emptyStepFlag == 1){
        finishedFlag = 0;
        step = 0;
        gridInit();
    }
}


void showFrameRate(){

    static double previousTime;
    static int frameCount;
    double currentTime = glfwGetTime();
    double deltaMs = (currentTime - previousTime) * 1000;
    frameCount++;
    if ( currentTime - previousTime >= 1.0 )
    {
        printf("FPS: %d\n", frameCount);
        frameCount = 0;
        previousTime = currentTime;
    }
}

void draw(){  
    showFrameRate();

    // Set the viewport   
    glViewport(0, 0, gSCR_HEIGHT, gSCR_WIDTH);   
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
        /*glBufferData(GL_ELEMENT_ARRAY_BUFFER, numGraphElements * sizeof(int), graphElements, GL_DYNAMIC_DRAW);*/
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, numGraphElements * sizeof(int), graphElements, GL_DYNAMIC_DRAW);

    }
}

void phelp()
{
    printf("Available params:\n");
    printf("-d      DENSITY; float 1.0 > DENSITY > 0)\n");
    printf("-df     DIM_FACTOR; float 1.0 > DIM_FACTOR > 0)\n");
    printf("-loop\n");

    exit(1);
}

void parse_args(int argc, char* argv[])
{
    printf("#args = %d\n", argc);
    for(int i = 1; i < argc; i+=2)
    {
        printf("%d: %s\n", i, argv[i]);
        if( 0 == strcmp("-d", argv[i]))
        {
            gGRID_DENSITY = atof(argv[i+1]);
            if(gGRID_DENSITY >= 1.0 || gGRID_DENSITY <= 0)
                phelp();
        }
        if( 0 == strcmp("-df", argv[i]))
        {
            gDIM_FACTOR = atof(argv[i+1]);
            if(gDIM_FACTOR >= 1.0 || gDIM_FACTOR <= 0)
                phelp();
        }
        if( 0 == strcmp("-loop", argv[i]))
        {
            loopFlag = 1;
        }
    }
}

int main(int argc, char *argv[])
{
	GLint status = 0;

    parse_args(argc, argv);

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

	window = glfwCreateWindow(gSCR_WIDTH, gSCR_HEIGHT, "Lightning", 0, 0);
	if (window == 0)
	{
		printf("Failed to create GLFW window\n");
		glfwTerminate();
		return 1;
	}
	
	glfwMakeContextCurrent(window);
    //glfwSwapInterval(0); //set to 0 for fastest FPS (not synchronized to screen refresh rate)
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

	glViewport(0, 0, gSCR_WIDTH, gSCR_HEIGHT);

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

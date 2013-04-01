#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <GL/glfw.h>

#define NUM_POINTS_X 20
#define NUM_POINTS_Y 20
#define NUM_POINTS (NUM_POINTS_X * NUM_POINTS_Y)
#define NUM_LINKS (NUM_POINTS_X * NUM_POINTS_Y * 2 - NUM_POINTS_X - NUM_POINTS_Y)
#define LINK_DIST 0.5

typedef struct point {
  float x, y, z;
  float prev_x, prev_y, prev_z;
  float vel_x, vel_y, vel_z;
  int anchor; //boolean, true if immovable
} Point;

typedef struct  link {
  float resting_distance;
  float tear_distance;
  int stiffness;
      
  Point *a;
  Point *b;

  int broken; //booelan, true if broken
} Link;
 
void init(void);
void shut_down(int return_code);
void main_loop();
void draw_square(Point **array);
void draw();
void update_point(Point *p, double delta);
void update_link(Link *l);
void check_boundaries(Point *p);
void update_mouse(Point **mp, int *mouse_held);
void draw_barrier();
 
Point *point_array[NUM_POINTS];

Link *link_array[NUM_LINKS];


float rotate_y = 0, rotate_z = 0;

 
int main(void)
{
  init();
  main_loop();
  shut_down(0);
}
 
void init(void)
{
  srand(time(0));

  const int window_width = 800,
    window_height = 600;

  int x = NUM_POINTS_X;
  int y = NUM_POINTS_Y;
 
  if (glfwInit() != GL_TRUE)
    shut_down(1);

  // 800 x 600, 16 bit color, no depth, alpha or stencil buffers, windowed
  if (glfwOpenWindow(window_width, window_height, 5, 6, 5,
                     0, 0, 0, GLFW_WINDOW) != GL_TRUE)
    shut_down(1);
  glfwSetWindowTitle("Cloth");
 
  // set the projection matrix to a normal frustum with a max depth of 50
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  float aspect_ratio = ((float)window_height) / window_width;
  glFrustum(.5, -.5, -.5 * aspect_ratio, .5 * aspect_ratio, 1, 50);
  glMatrixMode(GL_MODELVIEW);

  //create the Point grid
  for(int i = 0; i < y; i++) {
    for(int j = 0; j < x; j++) {
      Point *p = malloc(sizeof(Point));
      p->x = j*LINK_DIST;
      p->y = i*LINK_DIST + 5;//put them off the ground a bit
      p->z = 0.0;
      p->vel_x = 0.0;
      p->vel_y = 0.0;
      p->vel_z = 0.0;
      p->prev_x = p->x;
      p->prev_y = p->y;
      p->prev_z = p->z;
      p->anchor = 0;
      int invert = y-i-1; //inverts y axis
      if(invert == 0) {
	p->anchor = 1;
      }
      point_array[(invert*x)+j] = p;
    }
  }

  //link the points together in a mesh
  int q = 0;
  for(int i = 0; i < y; i++) {
    for(int j = 0; j < x; j++) {
      int invert = y-i-1;
      
      //link upward
      if(i != 0) { //not top row
	Link *l1 = malloc(sizeof(Link));
	l1->resting_distance = LINK_DIST;//10.0/x;
	l1->tear_distance = 15*LINK_DIST;
	l1->stiffness = 1;

	l1->a = point_array[invert*x+j];
	l1->b = point_array[(invert+1)*x+j];
	l1->broken = 0;
	link_array[q] = l1;
	q++;
      }

      //link leftward (is that a word?)
      if(j != 0) { //not left edge
	Link *l2 = malloc(sizeof(Link));
	l2->resting_distance = LINK_DIST;//10.0/x;
	l2->tear_distance = 15*LINK_DIST;
	l2->stiffness = 1;
      
	l2->a = point_array[invert*x+j];
	l2->b = point_array[invert*x+j-1];
	l2->broken = 0;
	link_array[q] = l2;
	q++;
      }
    }
  }
}
 
void shut_down(int return_code)
{
  glfwTerminate();
  exit(return_code);
}
 
void main_loop()
{
  // the time of the previous frame
  double old_time = glfwGetTime();
  // point that the mouse grabs
  Point *mouse_point = point_array[NUM_POINTS-1];
  int mouse_held = 0; // boolean
  // this just loops as long as the program runs
  while(1)
    {
      // calculate time elapsed
      double current_time = glfwGetTime(),
	delta_time = (current_time - old_time);
      old_time = current_time;
      // escape to quit, arrow keys to rotate view
      if (glfwGetKey(GLFW_KEY_ESC) == GLFW_PRESS) {
	break;
      }
      if (glfwGetKey(GLFW_KEY_LEFT) == GLFW_PRESS) {
	rotate_y += delta_time*30;
      }
      if (glfwGetKey(GLFW_KEY_RIGHT) == GLFW_PRESS) {
	rotate_y -= delta_time*30;
      }
      //randomly push the mesh in a random direction with SPACE
      if(glfwGetKey(GLFW_KEY_SPACE) == GLFW_PRESS) {
	int random_point = rand()%(NUM_POINTS-NUM_POINTS_X-1) + NUM_POINTS_X+1;
	Point *p = point_array[random_point];
	p->x += rand()%10000 / 10000.0 - .5;
	p->y += rand()%10000 / 10000.0 - .5;
	p->z += rand()%10000 / 10000.0 - .5;
      }

      //update link constraints multiple times
      for(int q = 0; q < 3; q++) {
	for(int i = 0; i < NUM_LINKS; i++) {
	  if(!link_array[i]->broken) {
	    update_link(link_array[i]);
	  }
	}      
      }

      //update all points except top row (fixed points)
      for(int i = 0; i < NUM_POINTS; i++) {
	if(!point_array[i]->anchor) {
	  update_point(point_array[i], delta_time);
	}
      }

      // clear the buffer
      glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
      // draw the figure
      draw(link_array);
      // mouse stuffs
      update_mouse(&mouse_point, &mouse_held);
      // swap back and front buffers
      glfwSwapBuffers();
    }
}
 
void draw_square(Point **array)
{
  // draws a square given array of 4 points
  glBegin(GL_QUADS);
  {
    for(int i = 0; i < 4; i++) {
      glVertex3f(array[i]->x, array[i]->y, array[i]->z);
    }
  }
  glEnd();
}
 
void draw()
{
  // reset view matrix
  glLoadIdentity();
  
  // move view back a bit and over
  glTranslatef(0, -5, -30);
  // draw a square for the mouse to hit (when clicked)
  draw_barrier();
  // apply rotation
  glRotatef(rotate_y, 0, 1, 0);
  glRotatef(rotate_z, 0, 0, 1);
  //move over a bit
  glTranslatef(-(NUM_POINTS_X-1)*LINK_DIST/2.0, 0, 0);

  //draw cloth grid
  glColor3f(0.0f, 1.0f, 0.0f);
  for (int i = 0; i < NUM_LINKS; i++) {
    if(!link_array[i]->broken) {
      Point *a = link_array[i]->a;
      Point *b = link_array[i]->b;
      glBegin(GL_LINES);
      glVertex3f(a->x, a->y, a->z);
      glVertex3f(b->x, b->y, b->z);
      glEnd();
    }
  }
}

void update_point(Point *p, double delta)
{
  //Euler version
  /*p->x += p->vel_x*delta;
    p->y += p->vel_y*delta;
    p->z += p->vel_z*delta;
    p->vel_y -= 0.98;//gravity*/
  
  //Verlet Integration
  p->vel_x = p->x - p->prev_x;
  p->vel_y = p->y - p->prev_y;
  p->vel_z = p->z - p->prev_z;
 
  //friction
  if(p->y <= 0.1) {
    p->vel_x *= 0.85;
    p->vel_z *= 0.85;
  }

  float next_x = p->x + p->vel_x;
  float next_y = p->y + p->vel_y - 0.98*delta;
  float next_z = p->z + p->vel_z;
 
  p->prev_x = p->x;
  p->prev_y = p->y;
  p->prev_z = p->z;
 
  p->x = next_x;
  p->y = next_y;
  p->z = next_z;

  check_boundaries(p);
}

void update_link(Link *l)
{
  // calculate the distance
  float diff_x = l->a->x - l->b->x;
  float diff_y = l->a->y - l->b->y;
  float diff_z = l->a->z - l->b->z;
  
  float d = sqrt(diff_x*diff_x + diff_y*diff_y + diff_z*diff_z);

  // tear
  if(d > l->tear_distance) {
    l->broken = 1;
    return;
  }

  // difference scalar
  float difference = 0;
  if(d != 0) { //DON'T DIVIDE BY ZERO!
    difference = (l->resting_distance - d) / d;
  }
 
  // translation for each PointMass. They'll be pushed 1/2 the required distance to match their resting distances.
  float translate_x = diff_x * 0.5 * difference;
  float translate_y = diff_y * 0.5 * difference;
  float translate_z = diff_z * 0.5 * difference;

  if(!l->a->anchor) {
    l->a->x += translate_x;
    l->a->y += translate_y;
    l->a->z += translate_z;
  }
 
  if(!l->b->anchor) {
    l->b->x -= translate_x;
    l->b->y -= translate_y;
    l->b->z -= translate_z;
  }
}

void check_boundaries(Point *p)
{
  if(p->x > 100) {
    p->x = 100;
    if(p->vel_x > 0) {
      p->vel_x *= -.9;
    }
  }
  if(p->x < -100) {
    p->x = -100;
    if(p->vel_x < 0) {
      p->vel_x *= -.9;
    }
  }

  if(p->y > 100) {
    p->y = 100;
    if(p->vel_y > 0) {
      p->vel_y *= -.9;
    }
  }
  if(p->y < 0) {
    p->y = 0;
    if(p->vel_y < 0) {
      p->vel_y *= -.9;
    }
  }

  if(p->z > 10) {
    p->z = 10;
    if(p->vel_z > 0) {
      p->vel_z *= -.9;
    }
  }
  if(p->z < -10) {
    p->z = -10;
    if(p->vel_z < 0) {
      p->vel_z *= -.9;
    }
  }
}

void update_mouse(Point **mp, int *mouse_held)
{
  // mp is a point to a pointer so that I can change
  // the pointer to save the point grabbed by the mouse
  // between function calls
  Point *mouse_point = *mp;
  if(!glfwGetMouseButton(GLFW_MOUSE_BUTTON_LEFT)) {
    *mouse_held = 0;
  }
  else {
    int x, y;
    glfwGetMousePos(&x, &y);

    GLint viewport[4];
    GLdouble modelview[16];
    GLdouble projection[16];
    GLfloat winX, winY;
    GLdouble posX, posY, posZ;
 
    glGetDoublev( GL_MODELVIEW_MATRIX, modelview );
    glGetDoublev( GL_PROJECTION_MATRIX, projection );
    glGetIntegerv( GL_VIEWPORT, viewport );
 
    winX = (float)x;
    winY = (float)(viewport[3] - y);
    
    gluUnProject( winX, winY, 0.99, modelview, projection, viewport, &posX, &posY, &posZ);

    //find nearest point to mouse click
    if(!(*mouse_held)) {
      *mouse_held = 1;
      Point *nearest;
      float d = 100000;
      for(int i = 0; i < NUM_POINTS; i++) {
	if(!point_array[i]->anchor) {
	  float diff_x = point_array[i]->x - posX;
	  float diff_y = point_array[i]->y - posY;
	  float diff_z = point_array[i]->z - posZ;
	  float dist = sqrt(diff_x*diff_x + diff_y*diff_y + diff_z*diff_z);
	  if(dist < d) {
	    d = dist;
	    nearest = point_array[i];
	  }
	}
      }
      *mp = nearest;
    }

    //drag point around
    mouse_point->x = posX;// - (NUM_POINTS_X-1)*LINK_DIST/2.0;
    mouse_point->y = posY;
    mouse_point->z = posZ;
  }
}

void draw_barrier()
{
  Point *p0 = malloc(sizeof(Point));
  p0->x = -100;
  p0->y = -100;
  p0->z = 0;

  Point *p1 = malloc(sizeof(Point));
  p1->x = -100;
  p1->y = 100;
  p1->z = 0;

  Point *p2 = malloc(sizeof(Point));
  p2->x = 100;
  p2->y = 100;
  p2->z = 0;

  Point *p3 = malloc(sizeof(Point));
  p3->x = 100;
  p3->y = -100;
  p3->z = 0;

  Point *array[] = {p0, p1, p2, p3};
  glColor4f(0, 0, 0, 0);
  draw_square(array);
}

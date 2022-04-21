#include <GL/gl.h>
#include <GL/glut.h>

void disp(void ) {
	glClear(GL_COLOR_BUFFER_BIT);

	glBegin(GL_TRIANGLES);
		glVertex2f(0 , 0);
		glVertex2f(-1 , 0.9);
		glVertex2f(1 , 0.9);

		glVertex2f(0 , 0);
		glVertex2f(-1 , -0.9);
		glVertex2f(1 , -0.9);
	glEnd();

    glFlush();
}

int main(int argc , char ** argv) {
	glutInit(&argc , argv);
	glutInitWindowPosition(100 , 50);
	glutInitWindowSize(500 , 500);
	glutInitDisplayMode(GLUT_SINGLE | GLUT_RGBA);

	glutCreateWindow("Kitty on your lap");
	glutDisplayFunc(disp);
	glutMainLoop();
	return 0;
}

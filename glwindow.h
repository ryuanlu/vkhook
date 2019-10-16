#ifndef	__GLWINDOW_H__
#define	__GLWINDOW_H__


struct glwindow* glwindow_create(void);
void glwindow_destroy(struct glwindow* glwindow);
void glwindow_run(struct glwindow* glwindow);


#endif	/* __GLWINDOW_H__ */
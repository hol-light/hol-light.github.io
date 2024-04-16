/*************************************************
*        Header for Wimp interface things        *
*************************************************/

/* Philip Hazel, August 1993 */

#include "kernel.h"



/*************************************************
*              SWI Numbers                       *
*************************************************/

#define Wimp_BlockCopy          0x400EB
#define Wimp_CloseDown          0x400DD
#define Wimp_CloseTemplate      0x400DA
#define Wimp_CloseWindow        0x400C6
#define Wimp_CreateIcon         0x400C2
#define Wimp_CreateMenu         0x400D4
#define Wimp_CreateWindow       0x400C1
#define Wimp_ForceRedraw        0x400D1
#define Wimp_DeleteWindow       0x400C3
#define Wimp_DragBox            0x400D0
#define Wimp_GetCaretPosition   0x400D3
#define Wimp_GetIconState       0x400CE
#define Wimp_GetPointerInfo     0x400CF
#define Wimp_GetRectangle       0x400CA
#define Wimp_GetWindowOutline   0x400E0
#define Wimp_GetWindowState     0x400CB
#define Wimp_Initialise         0x400C0
#define Wimp_LoadTemplate       0x400DB
#define Wimp_OpenTemplate       0x400D9
#define Wimp_OpenWindow         0x400C5
#define Wimp_Poll               0x400C7
#define Wimp_PollIdle           0x400E1
#define Wimp_ProcessKey         0x400DC
#define Wimp_ReadSysInfo        0x400F2
#define Wimp_RedrawWindow       0x400C8
#define Wimp_ReportError        0x400DF
#define Wimp_SendMessage        0x400E7
#define Wimp_SetCaretPosition   0x400D2
#define Wimp_SetColour          0x400E6
#define Wimp_SetExtent          0x400D7
#define Wimp_SetIconState       0x400CD
#define Wimp_SlotSize           0x400EC
#define Wimp_SpriteOp           0x400E9
#define Wimp_StartTask          0x400DE
#define Wimp_UpdateWindow       0x400C9




/*************************************************
*                   Flags                        *
*************************************************/

/* Flags for CreateMenu */

#define F_tick       0x001
#define F_dotted     0x002
#define F_writeable  0x004
#define F_message    0x008
#define F_allowgrey  0x010
#define F_last       0x080
#define F_indirect   0x100

/* Flags for icon blocks */

#define F_text       0x00000001
#define F_sprite     0x00000002
#define F_border     0x00000004
#define F_hcentre    0x00000008
#define F_vcentre    0x00000010
#define F_filled     0x00000020
#define F_antialias  0x00000040
#define F_taskhelp   0x00000080
#define F_indirected 0x00000100
#define F_justified  0x00000200
#define F_adjustesg  0x00000400
#define F_halfsize   0x00000800

#define F_selected   0x00200000
#define F_shaded     0x00400000
#define F_deleted    0x00800000

#define F_colours    0xFF000000
#define FS_fore      24           /* shifts */
#define FS_back      28
     




/*************************************************
*                  Structures                    *
*************************************************/


/******** Icons ********/

typedef struct {
  char *text;
  char *valid; 
  int  length;
} indirect_icon_data;    


typedef union {
  char name[12];
  indirect_icon_data ind;
} icon_data;


typedef struct {
  int minx;
  int miny;
  int maxx;
  int maxy;
  int flags;
  icon_data id;
} icon_block;


typedef struct {
  int handle;
  icon_block ib;
} create_icon_block;


typedef struct {
  int handle;
  int icon;
  icon_block ib;
} icon_state_block;     



/******** Windows ********/

typedef struct {
  int  minx;
  int  miny;
  int  maxx;
  int  maxy;
  int  scrollx;
  int  scrolly;
  int  behind;
  int  flags;
  char title_foreground;
  char title_background;
  char work_foreground;
  char work_background;
  char scroll_outer;
  char scroll_inner;
  char title_background_highlight;
  char dummy;
  int  work_minx;
  int  work_miny;
  int  work_maxx;
  int  work_maxy;
  int  title_iconflags;
  int  button_type;
  int *sprite_area;
  short int minwidth;
  short int minheight;                        
  icon_data title_data;
  int  iconcount;
  icon_block icons[9999];  
} window_data;


typedef struct {
  int handle;
  int minx;
  int miny;
  int maxx;
  int maxy;
  int scrollx;
  int scrolly;
  int infront;
  int flags;
} window_state_block;           


typedef struct {
  int handle;
  int minx;
  int miny;
  int maxx;
  int maxy;
  int scrollx;
  int scrolly;
  int gminx;
  int gminy;
  int gmaxx;
  int gmaxy;    
} window_redraw_block;           



/******** Menus ********/

typedef struct menu_block;

typedef struct menu_item {
  int flags;
  struct menu_block *submenu;
  int iflags;
  icon_data idata;
} menu_item;      


typedef struct indirect_menu_item {
  int flags;
  struct menu_block *submenu;
  int iflags;
  indirect_icon_data idata;
} indirect_menu_item;      


typedef struct menu_block {
  char title[12];
  char title_foreground;
  char title_background;
  char work_foreground;
  char work_background;
  int  item_width;
  int  item_height;      
  int  item_gap;
  menu_item items[1];
} menu_block;    



/******** Drag block ********/

typedef struct {
  int window;
  int type;
  int minx;
  int miny;
  int maxx;
  int maxy;
  int minxp;
  int minyp;
  int maxxp;
  int maxyp;
  int userR12;
  int drawboxroutine;
  int removeboxroutine;
  int moveboxroutine;
} drag_block;


/******** Caret position block *********/

typedef struct {
  int window;
  int icon;
  int xcaret;
  int ycaret;
  int caretflags; 
  int caretindex;
} caret_position;


/******** Message blocks ********/

/* The name length of 12 is suitable for DataSave, where it
contains only the leaf name. For others, the actual vector may
be longer. What a pity C doesn't allow variable length vectors
at the ends of structures... */

typedef struct {
  int length;      /*  0 */
  int sender;      /*  4 */
  int my_ref;      /*  8 */
  int your_ref;    /* 12 */
  int action;      /* 16 */
  int handle;      /* 20 */
  int icon;        /* 24 */
  int x;           /* 28 */
  int y;           /* 32 */
  int size;        /* 36 */
  int filetype;    /* 40 */
  char name[12];   /* 44 */
} msg_data_block;              

typedef struct {
  int length;      /*  0 */
  int sender;      /*  4 */
  int my_ref;      /*  8 */
  int your_ref;    /* 12 */
  int action;      /* 16 */
  int data_size;   /* 20 */
  char data[99999];
} msg_output_block;         

#define Message_Quit            0
#define Message_DataSave        1
#define Message_DataSaveAck     2
#define Message_DataLoad        3
#define Message_DataLoadAck     4
#define Message_DataOpen        5
#define Message_RAMFetch        6
#define Message_RAMTransmit     7
#define Message_PreQuit         8
#define Message_DataSaved      13

#define Message_TaskWindow_Input    0x808C0
#define Message_TaskWindow_Output   0x808C1
#define Message_TaskWindow_Ego      0x808C2
#define Message_TaskWindow_Morio    0x808C3
#define Message_TaskWindow_Morite   0x808C4
#define Message_TaskWindow_NewTask  0x808C5
#define Message_TaskWindow_Suspend  0x808C6
#define Message_TaskWindow_Resume   0x808C7






/*************************************************
*                Wimp_Poll data                  *
*************************************************/


/* Wimp_Poll return codes */

enum { Null_Reason_Code, Redraw_Window_Request, Open_Window_Request,
  Close_Window_Request, Pointer_Leaving_Window, Pointer_Entering_Window,
  Mouse_Click, User_Drag_Box, Key_Pressed, Menu_Selection, Scroll_Request,
  Lose_Caret, Gain_Caret, PollWord_NonZero, res1, res2, res3,
  User_Message, User_Message_Recorded, User_Message_Acknowledge };
  

/* Wimp_Poll mask bits & option bits */

#define M_Null_Reason_Code         0x00000001
#define M_Redraw_Window_Request    0x00000002  
#define M_Pointer_Leaving_Window   0x00000010
#define M_Pointer_Entering_Window  0x00000020
#define M_Mouse_Click              0x00000040
#define M_Key_Pressed              0x00000100
#define M_Lose_Caret               0x00000800
#define M_Gain_Caret               0x00001000
#define M_PollWord_NonZero         0x00002000
#define M_User_Message             0x00020000
#define M_User_Message_Recorded    0x00040000
#define M_User_Message_Acknowledge 0x00080000
#define M_All                      0x000E3973

#define M_PollWord                 0x00400000
#define M_Poll_HighPriority        0x00800000
#define M_Save_Floating_Point      0x01000000


/* Structures for return blocks from Wimp_Poll */

typedef struct {
  int x;
  int y;
  int buttons;
  int handle;
  int icon;
} S_Mouse_Click;       

typedef struct {
  int handle;
  int icon;
  int xcaret;
  int ycaret;
  int caretflags; 
  int caretindex;
  int key;
} S_Key_Pressed;



/************************************************
*                Variables                      *
************************************************/

extern _kernel_swi_regs  swi_regs;
extern char *Application_Name;      


/************************************************
*                Functions                      *
************************************************/

extern void display_error(char *, ...);
extern char *get_icon_text(int, int);
extern int  load_template(char *, window_data **);
extern void set_icon_text(int, int, char *);
     
/* End of wimphdr.h */

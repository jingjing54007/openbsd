/*	$OpenBSD: src/usr.bin/vim/proto/Attic/gui_x11.pro,v 1.1.1.1 1996/09/07 21:40:29 downsj Exp $	*/
/* gui_x11.c */
void gui_x11_timer_cb __PARMS((XtPointer timed_out, XtIntervalId *interval_id));
void gui_x11_visibility_cb __PARMS((Widget w, XtPointer dud, XEvent *event, Boolean *bool));
void gui_x11_expose_cb __PARMS((Widget w, XtPointer dud, XEvent *event, Boolean *bool));
void gui_x11_resize_window_cb __PARMS((Widget w, XtPointer dud, XEvent *event, Boolean *bool));
void gui_x11_focus_change_cb __PARMS((Widget w, XtPointer data, XEvent *event, Boolean *bool));
void gui_x11_key_hit_cb __PARMS((Widget w, XtPointer dud, XEvent *event, Boolean *bool));
void gui_x11_mouse_cb __PARMS((Widget w, XtPointer dud, XEvent *event, Boolean *bool));
void gui_mch_prepare __PARMS((int *argc, char **argv));
int gui_mch_init __PARMS((void));
void gui_mch_exit __PARMS((void));
void gui_x11_use_resize_callback __PARMS((Widget widget, int enabled));
int gui_mch_init_font __PARMS((char_u *font_name));
int gui_mch_haskey __PARMS((char_u *name));
int gui_get_x11_windis __PARMS((Window *win, Display **dis));
void gui_mch_beep __PARMS((void));
void gui_mch_flash __PARMS((void));
void gui_mch_iconify __PARMS((void));
void gui_mch_draw_cursor __PARMS((void));
void gui_mch_update __PARMS((void));
int gui_mch_wait_for_chars __PARMS((int wtime));
void gui_mch_flush __PARMS((void));
void gui_mch_clear_block __PARMS((int row1, int col1, int row2, int col2));
void gui_mch_outstr_nowrap __PARMS((char_u *s, int len, int wrap_cursor, int is_cursor));
void gui_mch_delete_lines __PARMS((int row, int num_lines));
void gui_mch_insert_lines __PARMS((int row, int num_lines));
void gui_request_selection __PARMS((void));
void gui_mch_lose_selection __PARMS((void));
int gui_mch_own_selection __PARMS((void));
void gui_x11_menu_cb __PARMS((Widget w, XtPointer client_data, XtPointer call_data));
void gui_x11_update_menus __PARMS((int modes));
void gui_x11_start_selection __PARMS((int button, int x, int y, int repeated_click, int_u modifiers));
void gui_x11_process_selection __PARMS((int button, int x, int y, int repeated_click, int_u modifiers));
void gui_x11_redraw_selection __PARMS((int x, int y, int w, int h));
void gui_mch_clear_selection __PARMS((void));

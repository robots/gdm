#ifndef UB_h_
#define UB_h_

extern struct usb_cdc_line_coding ub_cdc_line;

void ub_init(void);
void ub_periodic(void);
void ub_apply_setting(void);

#endif

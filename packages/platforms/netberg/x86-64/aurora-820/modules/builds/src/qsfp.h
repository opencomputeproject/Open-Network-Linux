//port info
#ifndef __SFF_QSFP_H
#define __SFF_QSFP_H

struct func_tbl_t *qsfp_func_load(void);
int sff_fsm_qsfp_task(struct sff_obj_t *sff_obj);
#endif /*__SFF_QSFP_H*/

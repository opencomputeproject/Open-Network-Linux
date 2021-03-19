//port info
#ifndef __SFF_SFP_H
#define __SFF_SFP_H

struct func_tbl_t *sfp_func_load(void);
int sff_fsm_sfp_task(struct sff_obj_t *sff_obj);
#endif /*__SFF_SFP_H*/

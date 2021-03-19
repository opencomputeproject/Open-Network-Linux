#ifndef __SFF_QSFP_DD_H
#define __SFF_QSFP_DD_H

int qsfp_dd_apsel_apply(struct sff_obj_t *sff_obj, int apsel);
int qsfp_dd_apsel_get(struct sff_obj_t *sff_obj);
int qsfp_dd_active_ctrl_set_get(struct sff_obj_t *sff_obj, char *buf, int size);
void qsfp_dd_rev4_quick_set(struct sff_obj_t *sff_obj, bool en);
bool qsfp_dd_rev4_quick_get(struct sff_obj_t *sff_obj);
int sff_fsm_qsfp_dd_task(struct sff_obj_t *sff_obj);
struct func_tbl_t *qsfp_dd_func_load(void);
#endif /*__SFF_QSFP_DD_H*/

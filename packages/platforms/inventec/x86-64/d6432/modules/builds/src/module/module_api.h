#ifndef __MODULE_API_H
#define __MODULE_API_H
/*the following function table and fsm_task will be exported as public fucntions for inv_swps use*/
/*sfp api*/
extern struct func_tbl_t sfp_func_tbl;
int sff_fsm_sfp_task(struct sff_obj_t *sff_obj);
/*qsfp api*/
extern struct func_tbl_t qsfp_func_tbl;
int sff_fsm_qsfp_task(struct sff_obj_t *sff_obj);
/*qsfp_dd api*/
extern struct func_tbl_t qsfp_dd_func_tbl;
int sff_fsm_qsfp_dd_task(struct sff_obj_t *sff_obj);
/*qsfp56 api*/
extern struct func_tbl_t qsfp56_func_tbl;
int sff_fsm_qsfp56_task(struct sff_obj_t *sff_obj);
/*sfp_dd api*/
extern struct func_tbl_t sfp_dd_func_tbl;
int sff_fsm_sfp_dd_task(struct sff_obj_t *sff_obj);

#endif /*__MODULE_API_H*/

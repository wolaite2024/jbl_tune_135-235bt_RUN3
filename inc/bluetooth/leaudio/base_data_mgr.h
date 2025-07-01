#ifndef _BASE_DATA_MGR_H_
#define _BASE_DATA_MGR_H_

#ifdef  __cplusplus
extern "C" {
#endif      /* __cplusplus */

#include <stdint.h>
#include "base_data_def.h"

#if LE_AUDIO_BASE_DATA_GENERATE
//source
bool base_data_add_group(uint8_t *p_group_idx, uint32_t presentation_delay);
bool base_data_del_group(uint8_t group_idx);
bool base_data_add_subgroup(uint8_t *p_subgroup_idx, uint8_t group_idx, uint8_t *codec_id,
                            uint8_t codec_cfg_len, uint8_t *p_codec_cfg,
                            uint8_t metadata_len, uint8_t *p_metadata);
bool base_data_add_bis(uint8_t *p_bis_idx, uint8_t group_idx, uint8_t subgroup_idx,
                       uint8_t codec_cfg_len, uint8_t *p_codec_cfg);
bool base_data_gen_pa_data(uint8_t group_idx, uint16_t *p_pa_len, uint8_t **pp_pa_data);
bool base_data_update_metadata(uint8_t group_idx, uint8_t subgroup_idx,
                               uint8_t metadata_len, uint8_t *p_metadata);

bool base_data_get_bis_num(uint8_t group_idx, uint8_t *p_bis_num);

#endif

#ifdef  __cplusplus
}
#endif      /*  __cplusplus */

#endif

/*
 *  Hamlib Kenwood backend - TH-D7 description
 *  Copyright (c) 2000-2010 by Stephane Fillod
 *
 *
 *   This library is free software; you can redistribute it and/or
 *   modify it under the terms of the GNU Lesser General Public
 *   License as published by the Free Software Foundation; either
 *   version 2.1 of the License, or (at your option) any later version.
 *
 *   This library is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *   Lesser General Public License for more details.
 *
 *   You should have received a copy of the GNU Lesser General Public
 *   License along with this library; if not, write to the Free Software
 *   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>

#include <hamlib/rig.h>
#include "kenwood.h"
#include "th.h"
#include "num_stdio.h"

#if 1
#define RIG_ASSERT(x)   if (!(x)) { rig_debug(RIG_DEBUG_ERR, "Assertion failed on line %i\n",__LINE__); abort(); }
#else
#define RIG_ASSERT(x)
#endif


#define THD7_MODES    (RIG_MODE_FM|RIG_MODE_AM)
#define THD7_MODES_TX (RIG_MODE_FM)

#define THD7_FUNC_ALL (RIG_FUNC_TSQL|   \
                       RIG_FUNC_AIP|    \
                       RIG_FUNC_MON|    \
                       RIG_FUNC_SQL|    \
                       RIG_FUNC_TONE|   \
                       RIG_FUNC_REV|    \
                       RIG_FUNC_LOCK|   \
                       RIG_FUNC_ARO)

#define THD7_LEVEL_ALL (RIG_LEVEL_STRENGTH| \
                        RIG_LEVEL_RAWSTR| \
                        RIG_LEVEL_SQL| \
                        RIG_LEVEL_RF|\
                        RIG_LEVEL_MICGAIN)

#define THD7_PARMS  (RIG_PARM_BACKLIGHT)

#define THD7_VFO_OP (RIG_OP_UP|RIG_OP_DOWN)

/*
 * TODO: Band A & B
 */
#define THD7_VFO (RIG_VFO_A|RIG_VFO_B)

static rmode_t td7_mode_table[KENWOOD_MODE_TABLE_MAX] =
{
    [0] = RIG_MODE_FM,
    [1] = RIG_MODE_AM,
};

static struct kenwood_priv_caps  thd7_priv_caps  =
{
    .cmdtrm =  EOM_TH,   /* Command termination character */
    .mode_table = td7_mode_table,
};

static rptr_shift_t thd7_rshf_table[3] =
{
    [0] = RIG_RPT_SHIFT_NONE,
    [1] = RIG_RPT_SHIFT_PLUS,
    [2] = RIG_RPT_SHIFT_MINUS,
    //[3] = -7.6MHZ ????
};



// This functions could be moved in th.c. Now are tested only on THD7A(G)
static int thd7_set_rptr_shft(RIG *rig, vfo_t vfo, rptr_shift_t rptr_shift)
{
    char cmd[8];
    rig_debug(RIG_DEBUG_TRACE, "%s: called\n", __func__);

    switch (rptr_shift)
    {
    case RIG_RPT_SHIFT_NONE:  strncpy(cmd, "SFT 0",sizeof cmd); break;

    case RIG_RPT_SHIFT_PLUS:  strncpy(cmd, "SFT 1", sizeof cmd); break;

    case RIG_RPT_SHIFT_MINUS: strncpy(cmd, "SFT 2", sizeof cmd); break;

    default:
        return  -RIG_EINVAL;
    }

    return kenwood_transaction(rig, cmd, cmd, sizeof cmd);
}

static int thd7_get_rptr_shft(RIG *rig, vfo_t vfo, rptr_shift_t *rptr_shift)
{
    int retval;
    char cmd[8], rsinx;
    rig_debug(RIG_DEBUG_TRACE, "%s: called\n", __func__);
    
    strncpy(cmd, "SFT", sizeof cmd);
    retval = kenwood_transaction(rig, cmd, cmd, sizeof cmd);
    if (retval != RIG_OK)
        {
                return retval;
        }else{
		rsinx=cmd[4];
	}
    *rptr_shift = (rsinx == '3') ? RIG_RPT_SHIFT_MINUS : thd7_rshf_table[(rsinx-'0')];
    return RIG_OK;
}


static int thd7_set_rptr_offs(RIG *rig, vfo_t vfo, shortfreq_t offs)
{
    char cmd[20];
    char offstr[9];
    rig_debug(RIG_DEBUG_TRACE, "%s: called\n", __func__);
    sprintf(offstr,"%ld",offs);
    size_t llen=12-strlen(offstr);
    strncpy(cmd,"OS 000000000",sizeof cmd);
    cmd[llen]='\0';
    strcat(cmd,offstr);
    return kenwood_transaction(rig, cmd, cmd, sizeof cmd);
}

static int thd7_get_rptr_offs(RIG *rig, vfo_t vfo, shortfreq_t *offs)
{
    int retval;
    char buf[64];

    rig_debug(RIG_DEBUG_TRACE, "%s: called\n", __func__);
    retval = kenwood_transaction(rig, "OS", buf, sizeof buf);

    if (retval != RIG_OK)
    {
        return retval;
    }
    num_sscanf(buf, "OS %ld", offs);
    return RIG_OK;
}

int
thd7_set_ptt(RIG *rig, vfo_t vfo, ptt_t ptt)
{
    
    //THD7A return TX <vcfo> ("TX 0" or "TX 1") 
    char buf[6];
    rig_debug(RIG_DEBUG_TRACE, "%s: called\n", __func__);
    return kenwood_transaction(rig, (ptt == RIG_PTT_ON) ? "TX" : "RX", buf,
                               sizeof buf);
}




/*
 * th-d7a rig capabilities.
 *
 * http://www.qsl.net/ta1dx/kenwood/thd7kom.htm
 */
const struct rig_caps thd7a_caps =
{
    RIG_MODEL(RIG_MODEL_THD7A),
    .model_name = "TH-D7A",
    .mfg_name =  "Kenwood",
    .version =  TH_VER ".0",
    .copyright =  "LGPL",
    .status =  RIG_STATUS_BETA,
    .rig_type =  RIG_TYPE_HANDHELD | RIG_FLAG_APRS | RIG_FLAG_TNC | RIG_FLAG_DXCLUSTER,
    .ptt_type =  RIG_PTT_RIG,
    .dcd_type =  RIG_DCD_RIG,
    .port_type =  RIG_PORT_SERIAL,
    .serial_rate_min =  9600,
    .serial_rate_max =  9600,
    .serial_data_bits =  8,
    .serial_stop_bits =  1,
    .serial_parity =  RIG_PARITY_NONE,
    .serial_handshake =  RIG_HANDSHAKE_NONE,
    .write_delay =  0,
    .post_write_delay =  0,
    .timeout =  200,
    .retry =  3,

    .has_get_func =  THD7_FUNC_ALL,
    .has_set_func =  THD7_FUNC_ALL,
    .has_get_level =  THD7_LEVEL_ALL,
    .has_set_level =  RIG_LEVEL_SET(THD7_LEVEL_ALL),
    .has_get_parm =  THD7_PARMS,
    .has_set_parm =  THD7_PARMS,    /* FIXME: parms */
    .level_gran = {
        // cppcheck-suppress *
        [LVL_RAWSTR] = { .min = { .i = 0 }, .max = { .i = 5 } },
        [LVL_SQL] = { .min = { .i = 0 }, .max = { .i = 5 } },
        [LVL_RFPOWER] = { .min = { .i = 3 }, .max = { .i = 0 } },
    },
    .parm_gran =  {},
    .ctcss_list =  kenwood38_ctcss_list,
    .dcs_list =  NULL,
    .preamp =   { RIG_DBLST_END, },
    .attenuator =   { RIG_DBLST_END, },
    .max_rit =  Hz(0),
    .max_xit =  Hz(0),
    .max_ifshift =  Hz(0),
    .vfo_ops =  THD7_VFO_OP,
    .targetable_vfo =  RIG_TARGETABLE_FREQ,
    .transceive =  RIG_TRN_RIG,
    .bank_qty =   0,
    .chan_desc_sz =  6,

    .chan_list =  {
        {  1,  199, RIG_MTYPE_MEM, {TH_CHANNEL_CAPS}},   /* normal MEM */
        {  200, 219, RIG_MTYPE_EDGE, {TH_CHANNEL_CAPS}}, /* U/L MEM */
        {  221, 222, RIG_MTYPE_CALL, {TH_CHANNEL_CAPS}}, /* Call 0/1 */
        RIG_CHAN_END,
    },

    .rx_range_list1 =  { RIG_FRNG_END, },    /* FIXME: enter region 1 setting */
    .tx_range_list1 =  { RIG_FRNG_END, },
    .rx_range_list2 =  {
        {MHz(144), MHz(148), THD7_MODES, -1, -1, THD7_VFO},
        {MHz(430), MHz(440), THD7_MODES, -1, -1, THD7_VFO},
        RIG_FRNG_END,
    }, /* rx range */
    .tx_range_list2 =  {
        {MHz(144), MHz(148), THD7_MODES_TX, W(5.5), W(5.5), THD7_VFO},
        {MHz(430), MHz(440), THD7_MODES_TX, W(5.5), W(5.5), THD7_VFO},
        RIG_FRNG_END,
    }, /* tx range */
    .tuning_steps =  {
        {THD7_MODES, kHz(5)},
        {THD7_MODES, kHz(6.25)},
        {THD7_MODES, kHz(10)},
        {THD7_MODES, kHz(12.5)},
        {THD7_MODES, kHz(15)},
        {THD7_MODES, kHz(20)},
        {THD7_MODES, kHz(25)},
        {THD7_MODES, kHz(30)},
        {THD7_MODES, kHz(50)},
        {THD7_MODES, kHz(100)},
        RIG_TS_END,
    },
    /* mode/filter list, remember: order matters! */
    .filters =  {
        {RIG_MODE_FM, kHz(14)},
        {RIG_MODE_AM, kHz(9)},
        RIG_FLT_END,
    },
    .priv = (void *)& thd7_priv_caps,

    .rig_init = kenwood_init,
    .rig_open = kenwood_open,
    .rig_close = kenwood_close,
    .rig_cleanup = kenwood_cleanup,
    .set_freq =  th_set_freq,
    .get_freq =  th_get_freq,
    .set_mode =  th_set_mode,
    .get_mode =  th_get_mode,
    .set_vfo =  th_set_vfo,
    .get_vfo =  th_get_vfo,
    .set_ctcss_tone =  th_set_ctcss_tone,
    .get_ctcss_tone =  th_get_ctcss_tone,
    .set_ctcss_sql =  th_set_ctcss_sql,
    .get_ctcss_sql =  th_get_ctcss_sql,
    .set_mem =  th_set_mem,
    .get_mem =  th_get_mem,
    .set_channel =  th_set_channel,
    .get_channel =  th_get_channel,
    .set_trn =  th_set_trn,
    .get_trn =  th_get_trn,

    .get_func =  th_get_func,
    .get_level =  th_get_level,
    .get_parm =  th_get_parm,
    .get_info =  th_get_info,

    .get_dcd =  th_get_dcd,

    .decode_event =  th_decode_event,

    //experimental 
    .set_rptr_shift = thd7_set_rptr_shft,
    .get_rptr_shift = thd7_get_rptr_shft,
    .set_rptr_offs = thd7_set_rptr_offs,
    .get_rptr_offs = thd7_get_rptr_offs,
    .set_ptt  = thd7_set_ptt,
};




/* end of file */


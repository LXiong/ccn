/*
 *
 * Copyright (c) 2006, Graduate School of Niigata University,
 *                                         Ad hoc Network Lab.
 * Developer:
 *  Yasunori Owada  [yowada@net.ie.niigata-u.ac.jp],
 *  Kenta Tsuchida  [ktsuchi@net.ie.niigata-u.ac.jp],
 *  Taka Maeno      [tmaeno@net.ie.niigata-u.ac.jp],
 *  Hiroei Imai     [imai@ie.niigata-u.ac.jp].
 * Contributor:
 *  Keita Yamaguchi [kyama@net.ie.niigata-u.ac.jp],
 *  Yuichi Murakami [ymura@net.ie.niigata-u.ac.jp],
 *  Hiraku Okada    [hiraku@ie.niigata-u.ac.jp].
 *
 * This software is available with usual "research" terms
 * with the aim of retain credits of the software.
 * Permission to use, copy, modify and distribute this software for any
 * purpose and without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies,
 * and the name of NIIGATA, or any contributor not be used in advertising
 * or publicity pertaining to this material without the prior explicit
 * permission. The software is provided "as is" without any
 * warranties, support or liabilities of any kind.
 * This product includes software developed by the University of
 * California, Berkeley and its contributors protected by copyrights.
 */


#include "def.h"
#include "packet.h"
#include "olsr_util_inline.h"

/**
 *Free the memory allocated for a HELLO packet.
 *
 *@param message the pointer to the packet to erase
 *
 *@return nada
 */
void
olsr_free_hello_packet(struct hello_message *message)
{
  struct hello_neighbor *nb = NULL, *prev_nb = NULL;

  if(!message)
    return;

  //free local interface block
  OLSR_DeleteList_Static(&message->local_iface_list);

  nb = message->neighbors;

  while (nb)
    {
      prev_nb = nb;
      nb = nb->next;
      free(prev_nb);
    }
}


/**
 *Free the memory allocated for a TC packet.
 *
 *@param message the pointer to the packet to erase
 *
 *@return nada
 */



void
olsr_free_tc_packet(struct tc_message *message)
{
  struct tc_mpr_addr *mprs = NULL, *prev_mprs = NULL;
  ATTACHED_NETWORK *tmp = NULL, *del = NULL;

  if(!message)
    return;

  //free local interface block
  OLSR_DeleteList_Static(&message->local_iface_list);

  mprs = message->mpr_selector_address;

  while (mprs)
    {
      prev_mprs = mprs;
      mprs = mprs->next;
      free(prev_mprs);
    }

  tmp = message->attached_net_addr;
  while(tmp)
    {
      del = tmp;
      tmp = tmp->next;
      free(del);
    }

}



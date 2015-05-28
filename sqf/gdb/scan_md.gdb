#
# gdb macro to scan the seabed md's
#

define scan_md
  set $inx = -1
  set $maxmds = SB_Trans::Msg_Mgr::cv_md_table.iv_cap
  set $mdp = SB_Trans::Msg_Mgr::cv_md_table.ipp_table

  printf "\nSize of table: %u entries\n", $maxmds
  while (++$inx < $maxmds)
    if ($mdp[$inx].iv_inuse != 0)
      printf "md index :  0x%x\n", $inx
      x/s $mdp[$inx]->ip_stream
      p $mdp[$inx]->ip_where
    end
  end
  printf "\n"
end

document scan_md
scan_md

prints all the seabed md' that are in use
end

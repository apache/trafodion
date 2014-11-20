define psb
	help psb
end

document psb
	<cmd>         - <args>  - <description>
	psbassert     - none    - print sb assert
	psbbt         - none    - print sb bt on all threads
	psbdq         - none    - print sb doubly-linked-queue
	psbenviron    - none    - print sb environ
	psberrno      - errno   - print sb errno
	psbevents     - none    - print sb events
	psbfd         - fnum    - print sb fd
	psbfserror    - error   - print sb fs-error
	psbfsopens    - none    - print sb fs-opens
	psbid         - none    - print sb identity
	psbimap       - map     - print sb imap (int map)
	psblmap       - map     - print sb lmap (long map)
	psbllmap      - map     - print sb llmap (long long map)
	psblocio      - none    - print sb local i/o
	psbmd         - msgid(s)- print sb md(s)
	psbmdall      - ts/delta- print sb all mds
	psbmdmap      - map     - print sb mdmap
	psbmds        - ts/delta- print sb mds
	psbmsctrl     - addr    - print sb ms-control
	psbmsopens    - none    - print sb ms-opens
	psbnidpidmap  - none    - print sb nidpidmap (nidpid/stream map)
	psbod         - oid     - print sb od
	psbover       - none    - print sb overview
	psboverwfs    - none    - print sb overview with fs
	psbrecvq      - none    - print sb recv-q
	psbrmmap      - map     - print sb rmmap (reqid/msgid map)
	psbslotmgr    - slotmgr - print sb slot-mgr
	psbsmap       - map     - print sb smap (string map)
	psbstreamdelq - none    - print sb stream-del-q
	psbstreams    - none    - print sb streams
	psbtable      - table   - print sb table
	psbthreads    - none    - print sb threads
	psbtimermap   - map     - print sb timermap (timer map)
	psbtimer      - time    - print sb timer
	psbtimers     - none    - print sb timers
	psbtracemem   - #recs   - print sb trace mem
	psbutraceapi  - #recs   - print sb utrace-api
	psbutracempi  - #recs   - print sb utrace-mpi
	psbutraceprof - #recs   - print sb utrace-test-profiler
end

define psbaintevent
	set $evevent = $arg0
	set $evmapinx = 0
	set $evmask = 0x8000
	set $evinx = 0
	printf "( "
	while $evinx < 16
		if $evevent & $evmask
			printf "%s ", ga_event_map[$evmapinx]
		end
		set $evmask = $evmask >> 1
		set $evmapinx++
		set $evinx++
	end
	printf ")"
end

define psbaintlabelget
	set $getlabels = $arg0
	set $getinx = $arg1
	set $getlow = $getlabels.iv_low_inx
	set $getmax = $getlabels.iv_max - $getlow
	set $getinx -= $getlow
	if ($getinx >= 0) && ($getinx <= $getmax)
		set $label = $getlabels.ipp_labels[$getinx]
	else
		set $label = $getlabels.ip_unknown
	end
end

define psbaintlabelgetmaps
	set $getlabelsa = $arg0
	set $getlabels = $getlabelsa[0]
	set $getinxa = 1
	set $label = 0
	while $getlabels != 0
		set $getinx = $arg1
		set $getlow = $getlabels->iv_low_inx
		set $getmax = $getlabels->iv_max - $getlow
		set $getinx -= $getlow
		if ($getinx >= 0) && ($getinx <= $getmax)
			set $getlist = (char *[]) $getlabels->ipp_labels
			set $label = $getlabels->ipp_labels[$getinx]
			set $getlabels = 0
		else
			set $getlabels = $getlabelsa[$getinxa]
			set $getinxa++
		end
	end
	if $label == 0
		set $label = $getlabelsa[0]->ip_unknown
	end
end

define psbaintmdfmtslot
	set $state = $md->iv_md_state
	psbaintlabelget gv_sb_stream_md_state_label_map $state
	printf ", state=%d(%s), send-done=%d, slot-hdr=%d, ctrl=%d, data=%d\n", $state, $label, $md->iv_ss.iv_send_done, $md->iv_ss.iv_slot_hdr, $md->iv_ss.iv_slot_ctrl, $md->iv_ss.iv_slot_data
end

define psbaintmdfmtts
	set $state = $md->iv_md_state
	psbaintlabelget gv_sb_stream_md_state_label_map $state
	printf ", state=%d(%s)", $state, $label
	set $delta_ts_min = 0x7fffffff00000000LL
	set $delta_ts_max = 0
	if $md->iv_ts_msg_cli_link.tv_sec != 0
		set $delta_ts = (long long) $md->iv_ts_msg_cli_link.tv_sec * 1000000 + $md->iv_ts_msg_cli_link.tv_usec
		if $delta_ts < $delta_ts_min
			set $delta_ts_min = $delta_ts
		end
		if $delta_ts > $delta_ts_max
			set $delta_ts_max = $delta_ts
		end
		psbainttsget $md->iv_ts_msg_cli_link.tv_sec $md->iv_ts_msg_cli_link.tv_usec
		printf ", link=%02d:%02d:%02d.%06d", $ts_hr, $ts_min, $ts_sec, $ts_usec
	end
	if $md->iv_ts_msg_srv_rcvd.tv_sec != 0
		set $delta_ts = (long long) $md->iv_ts_msg_srv_rcvd.tv_sec * 1000000 + $md->iv_ts_msg_srv_rcvd.tv_usec
		if $delta_ts < $delta_ts_min
			set $delta_ts_min = $delta_ts
		end
		if $delta_ts > $delta_ts_max
			set $delta_ts_max = $delta_ts
		end
		psbainttsget $md->iv_ts_msg_srv_rcvd.tv_sec $md->iv_ts_msg_srv_rcvd.tv_usec
		printf ", rcvd=%02d:%02d:%02d.%06d", $ts_hr, $ts_min, $ts_sec, $ts_usec
	end
	if $md->iv_ts_msg_srv_listen.tv_sec != 0
		set $delta_ts = (long long) $md->iv_ts_msg_srv_listen.tv_sec * 1000000 + $md->iv_ts_msg_srv_listen.tv_usec
		if $delta_ts < $delta_ts_min
			set $delta_ts_min = $delta_ts
		end
		if $delta_ts > $delta_ts_max
			set $delta_ts_max = $delta_ts
		end
		psbainttsget $md->iv_ts_msg_srv_listen.tv_sec $md->iv_ts_msg_srv_listen.tv_usec
		printf ", list=%02d:%02d:%02d.%06d", $ts_hr, $ts_min, $ts_sec, $ts_usec
	end
	if $md->iv_ts_msg_srv_reply.tv_sec != 0
		set $delta_ts = (long long) $md->iv_ts_msg_srv_reply.tv_sec * 1000000 + $md->iv_ts_msg_srv_reply.tv_usec
		if $delta_ts < $delta_ts_min
			set $delta_ts_min = $delta_ts
		end
		if $delta_ts > $delta_ts_max
			set $delta_ts_max = $delta_ts
		end
		psbainttsget $md->iv_ts_msg_srv_reply.tv_sec $md->iv_ts_msg_srv_reply.tv_usec
		printf ", reply=%02d:%02d:%02d.%06d", $ts_hr, $ts_min, $ts_sec, $ts_usec
	end
	if $md->iv_ts_msg_cli_rcvd.tv_sec != 0
		set $delta_ts = (long long) $md->iv_ts_msg_cli_rcvd.tv_sec * 1000000 + $md->iv_ts_msg_cli_rcvd.tv_usec
		if $delta_ts < $delta_ts_min
			set $delta_ts_min = $delta_ts
		end
		if $delta_ts > $delta_ts_max
			set $delta_ts_max = $delta_ts
		end
		psbainttsget $md->iv_ts_msg_cli_rcvd.tv_sec $md->iv_ts_msg_cli_rcvd.tv_usec
		printf ", rcvd=%02d:%02d:%02d.%06d", $ts_hr, $ts_min, $ts_sec, $ts_usec
	end
	if $md->iv_ts_msg_cli_break.tv_sec != 0
		set $delta_ts = (long long) $md->iv_ts_msg_cli_break.tv_sec * 1000000 + $md->iv_ts_msg_cli_break.tv_usec
		if $delta_ts < $delta_ts_min
			set $delta_ts_min = $delta_ts
		end
		if $delta_ts > $delta_ts_max
			set $delta_ts_max = $delta_ts
		end
		psbainttsget $md->iv_ts_msg_cli_break.tv_sec $md->iv_ts_msg_cli_break.tv_usec
		printf ", break=%02d:%02d:%02d.%06d", $ts_hr, $ts_min, $ts_sec, $ts_usec
	end
	if $odelta = 1
		if $delta_ts_max != 0
			set $delta_ts = $delta_ts_max - $delta_ts_min
			set $delta_ts_max_sec = $delta_ts / 1000000
			set $delta_ts_max_usec = $delta_ts % 1000000
			printf ", delta=%d.%06d", $delta_ts_max_sec, $delta_ts_max_usec
		end
	end
	printf "\n"
end

define psbaintthreadname
	set $tntid = $arg0
	set $tncap = gv_sb_thread_table.iv_cap
	set $tninuse = gv_sb_thread_table.iv_inuse
	set $tncount = 0
	set $tninx = 0
	set $tnname = 0
	while $tninx < $tncap
		set $tnthread = gv_sb_thread_table.ipp_table[$tninx]
		if $tnthread != 0
			if $tntid == $tnthread->iv_tid
				set $tnname = $tnthread->ia_thread_name
				set $tninx = $tncap
			end
		end
		set $tncount++
		if $tncount >= $tninuse
			set $tninx = $tncap
		end
		set $tninx++
	end
end

define psbaintthreadstls
	set $tlsselfin = $arg0
	set $tlsself = (struct SB_TLS_pthread *) $tlsselfin
	set $tlsinx = 0
	while $tlsinx < 32
		set $seq = $tlsself->specific1[$tlsinx].seq
		set $data = $tlsself->specific1[$tlsinx].data
		if $tlsinx, $seq != 0
			if $tlsinx < 0
				set $tlsnamei = gv_sb_tls_key_max
			else
				set $tlsnamei = $tlsinx
			end
			if $tlsnamei < gv_sb_tls_key_max
				set $tlsname = ga_sb_tls_keys[$tlsnamei].ip_desc
			else
				set $tlsname = gp_sb_tls_key_null
			end
			if $tlsname == gp_sb_tls_key_null
				printf "  tls[%d] data=0x%x\n", $tlsinx, $seq
			else
				printf "  tls[%d] data=0x%x, name=%s\n", $tlsinx, $seq, $tlsname
			end
			if $tlsinx == $timer_tls_inx
				set $compq = (SB_Timer_Comp_Queue *) $data
				if $data != 0
					set $tle = (Timer_TLE_Type *) $compq->ip_head
					set $tinx = 0
					while $tle != 0
						psbainttle 1
						set $tle = $tle->ip_next
					end
				end
			end
		end
		set $tlsinx++
	end
end

define psbainttle
	set $tleindent = $arg0
	set $kind = $tle->iv_kind
	psbaintlabelget gv_timer_kind_label_map $kind
	set $label_kind = $label
	set $on_list = $tle->iv_on_list
	psbaintlabelget gv_timer_list_label_map $on_list
	set $label_on_list = $label
	if $tleindent
		printf "    "
	end
	printf "tle[%d]=%p, next=%p, prev=%p, tleid=%d, inuse=%d, kind=%d(%s), on-list=%d(%s), cb=%p, stid=%d, ttid=%d, to=%lld, parm1=%d(%x), parm2=%ld(0x%lx)\n", $tinx, $tle, $tle->ip_next, $tle->ip_prev, $tle->iv_tleid, $tle->iv_inuse, $kind, $label_kind, $on_list, $label_on_list, $tle->iv_cb, $tle->iv_stid, $tle->iv_ttid, $tle->iv_to, $tle->iv_parm1, $tle->iv_parm1, $tle->iv_parm2, $tle->iv_parm2
end

define psbainttsget
	set $ts_sec = $arg0
	set $ts_usec = $arg1
	set $ts_sec = $ts_sec % 86400
	set $ts_hr = $ts_sec / 3600
	set $ts_sec %= 3600
	set $ts_min = $ts_sec / 60
	set $ts_sec = $ts_sec % 60
end

define psbaintutraceapirec
	set $precinx = $arg0
	set $prec = $arg1
	set $precop = $prec.iv_op
	psbaintlabelget gv_sb_utrace_api_op_label_map $precop
	printf "rec[%05d]: tid=%d, op=%d(%s), id=%d(0x%x), info1=%d(0x%x)\n", $precinx, $prec.iv_tid, $precop, $label, $prec.iv_id, $prec.iv_id, $prec.iv_info1, $prec.iv_info1
end

define psbaintutracempirec
	set $precinx = $arg0
	set $prec = $arg1
	set $precop = $prec.iv_op
	psbaintlabelget gv_sb_utrace_mpi_op_label_map $precop
	printf "rec[%05d]: tid=%d, op=%d(%s), id1=%d(0x%x), id2=%d(0x%x)\n", $precinx, $prec.iv_tid, $precop, $label, $prec.iv_id1, $prec.iv_id1, $prec.iv_id2, $prec.iv_id2
end

define psbassert
	printf "psbassert - gv_ms_save_assert\n"
	printf "cmd=%s\n", gv_ms_save_assert.ia_cmdline
	printf "buf=%s", gv_ms_save_assert.ia_buf
	printf "errno=%d\n", gv_ms_save_assert.iv_errno
	set $lhs = gv_ms_save_assert.iv_lhs
	set $rhs = gv_ms_save_assert.iv_rhs
	printf "lhs=%d (0x%x), rhs=%d (0x%x)\n", $lhs, $lhs, $rhs, $rhs
	printf "\n"
end

define psbbt
	printf "psbbt\n"
	thread apply all bt
end

define psbcompq
	set $map = &gv_ms_ldone_map
	set $hash = 0
	set $hash_end = $map->iv_buckets
	printf "psbcompq - count=%d\n", $map->iv_count
	set $i = 0
	while $hash < $hash_end
		set $node = $map->ipp_HT[$hash]
		while $node != 0
			set $compq = (SB_Comp_Queue *) $node->iv_id.l
			set $count = $compq->iv_count
			set $tid = $compq->iv_tid
			psbaintthreadname $tid
			printf "compq[%d] tid=%d", $i, $tid
			if $tnname != 0
				printf " %s", $tnname
			end
			printf ", count=%d\n", $count
			set $inx = 0
			set $mdl = $compq->ip_head
			while $inx < $count
				set $md = (MS_Md_Type *) $mdl
				set $stream = ('SB_Trans::Sock_Stream' *) $md->ip_stream
				printf "  md[%d]=%p, stream=%p ", $mdl->iv_id.i, $mdl, $stream
				if $stream == 0
					printf "?"
				else
					printf "%s", $stream->ia_stream_name
				end
				printf ", where="
				if $md->ip_where == 0
					printf "?"
				else
					printf "%s", $md->ip_where
				end
				printf "\n"
				set $mdl = $mdl->ip_next
				set $inx++
			end
			set $node = $node->ip_next
			set $i++
		end
		set $hash++
	end
	printf "\n"
end

define psbdq
	if $argc == 0
		help psbdq
	else
		set $q = $arg0
		printf "psbdq=%p, name=%s, qid=%d, count=%d, h=%p, t=%p\n", $q, $q->ia_d_q_name, $q->iv_qid, $q->iv_count, $q->ip_head, $q->ip_tail
		set $p = $q->ip_head
		set $inx = 0
		while $p != 0
			set $idi = $p->iv_id.i
			set $idl = $p->iv_id.l
			set $idll = $p->iv_id.ll
			printf "p[%d]=%p, n=%p, p=%p, qid=%d, lqid=%d, id.i=%d(0x%x), .l=%ld(0x%lx), .ll=%lld(0x%llx)\n", $inx, $p, $p->ip_next, $p->ip_prev, $p->iv_qid, $p->iv_qid_last, $idi, $idi, $idl, $idl, $idll, $idll
			set $p = $p->ip_next
			set $inx++
		end
	end
end

document psbdq
	Prints SB_D_Queue (doubly-linked queue)
	Syntax: psbdq <dq>
	Example:
	psbdq dq - Prints sb dq
end

define psbevents
	set $max = SB_Ms_Event_Mgr::cv_all_list.iv_count
	set $entry = SB_Ms_Event_Mgr::cv_all_list.ip_head
	set $inx = 0
	printf "psbevents - SB_Ms_Event_Mgr::cv_all_list: count=%d\n", $max
	while $inx < $max
		set $entryt = ('SB_Ms_Event_Mgr::Map_All_Entry_Type' *) $entry
		set $mgr = $entryt->ip_mgr
		set $awake = $mgr->iv_awake
		printf "mgr[%d]=%p, tid=%d, group=%d, pin=%d, replies=%d, awake=0x%x", $inx, $mgr, $mgr->iv_id, $mgr->iv_group, $mgr->iv_pin, $mgr->iv_replies, $awake
		psbaintevent $awake
		printf "\n"
		set $entry = $entry->ip_next
		set $inx++
	end
	printf "\n"
end

define psbenviron
	printf "psbenviron\n"
	set $inx = 0
	while *((char **) environ+$inx)
		printf "%s\n", *((char **) environ+$inx)
		set $inx++
	end
	printf "\n"
end

define psbenvvars
	printf "psbenvvars\n"
	printf "gv_ms_shutdown_fast=%d\n", gv_ms_shutdown_fast
	printf "gv_ms_max_phandles=%d\n", gv_ms_max_phandles
	printf "gv_ms_streams_max=%d\n", gv_ms_streams_max
	printf "\n"
end

define psberrno
	if $argc == 0
		help psberrno
	else
		set $lerrno = $arg0
		psbaintlabelget gv_ms_errno_type_label_map $lerrno
		printf "%s\n", $label
	end
end

document psberrno
	Prints errno-error
	Syntax: psberrno <errno>
	Example:
	psberrno 0   - Prints EPERM
end

define psbfd
	if $argc == 0
		help psbfd
	else
		set $fnum = $arg0
		set $cap = gv_fs_filenum_table.iv_cap
		set $table = gv_fs_filenum_table.ipp_table
		if $fnum >= $cap
			printf "fnum (%d) is invalid (bigger than cap=%d)\n", $fnum, $cap
			set $fnum = $cap
		end
		if $fnum < 0
			printf "fnum (%d) is invalid (negative)\n", $fnum
			set $fnum = $cap
		end
		if $fnum != $cap
			printf "fnum=%d\n", $fnum
			if $table[$fnum] == 0
				printf "fd is NULL\n"
			else
				p *$table[$fnum]
			end
		end
	end
end

document psbfd
	Prints fd
	Syntax: psbfd <fnum>
	Example:
	psbfd 1   - Prints sb fd 1
end

define psbfserror
	if $argc == 0
		help psbfserror
	else
		set $fserror = $arg0
		psbaintlabelgetmaps ga_ms_fserror_type_label_map $fserror
		printf "%s\n", $label
	end
end

document psbfserror
	Prints fs-error
	Syntax: psbfserror <error>
	Example:
	psbfserror 0   - Prints FEOK
end

define psbfsopens
	set $cap = gv_fs_filenum_table.iv_cap
	set $inuse = gv_fs_filenum_table.iv_inuse
	set $count = 1
	set $inx = 0
	if $inuse == 0
		set $inx = $cap + 1
	end
	printf "psbfsopens - gv_fs_filenum_table: cap=%d, inuse=%d\n", $cap, $inuse
	while $inx <= $cap
		set $fd = gv_fs_filenum_table.ipp_table[$inx]
		if $fd && $fd->iv_inuse
			if $fd->ip_ru_tag_mgr == 0
				set $rumax = $cap
			else
				set $rumax = $fd->ip_ru_tag_mgr->iv_max
			end
			printf "fd[%d]=%p, fname=%s, options=%d, nowait-depth=%d, nowait-open=%d, op-depth=%d, recv-depth=%d, oid=%d, type=%d, ru-max=%d\n", $inx, $fd, $fd->ia_fname, $fd->iv_options, $fd->iv_nowait_depth, $fd->iv_nowait_open, $fd->iv_op_depth, $fd->iv_recv_depth, $fd->iv_oid, $fd->iv_file_type, $rumax
			set $count++
			if $count >= $inuse
				set $inx = $cap
			end
			set $ruinx = 0
			while $ruinx < $fd->iv_recv_depth
				set $ru = &$fd->ip_ru[$ruinx]
				if $ru->iv_inuse
					printf "  ru[%d]=%p, buffer=%p, rc=%d, cw=%d, ru-tag=%d\n", $ruinx, $ru, $ru->ip_buffer, $ru->iv_read_count, $ru->iv_count_written, $ru->iv_ru_tag
				end
				if $ruinx >= $rumax
					set $ruinx = $fd->iv_recv_depth
				end
				set $ruinx++
			end
			set $ioinx = 0
			while $ioinx < $fd->iv_nowait_depth
				set $io = &$fd->ip_io[$ioinx]
				if $io->iv_inuse
					printf "  io[%d]=%p, buffer=%p, msgid=%d, tag-user=%d\n", $ioinx, $io, $io->ip_buffer, $io->iv_msgid, $io->iv_tag_user
				end
				set $ioinx++
			end
		end
		set $inx++
	end
	printf "\n"
end

define psbid
	printf "psbid\n"
	printf "pname=%s, prog=%s, nid=%d, pid=%d, pnid=%d, ptype=%d, port=%s\n", ga_ms_su_pname, ga_ms_su_prog, gv_ms_su_nid, gv_ms_su_pid, gv_ms_su_pnid, gv_ms_su_ptype, ga_ms_su_a_port
	printf "\n"
end

define psbimap
	if $argc == 0
		help psbimap
	else
		set $imap = $arg0
		set $hash = 0
		set $hash_end = $imap->iv_buckets
		printf "psbimap=%p, name=%s, count=%d, buckets=%d\n", $imap, $imap->ia_map_name, $imap->iv_count, $hash_end
		while $hash < $hash_end
			set $node = $imap->ipp_HT[$hash]
			set $i = 0
			while $node != 0
				set $k = $node->iv_id.i
				printf "h=%d, c=%d: k=%d(0x%x)\n", $hash, $i, $k, $k
				set $node = $node->ip_next
				set $i++
			end
			set $hash++
		end
	end
end

document psbimap
	Prints imap (int map)
	Syntax: psbimap <imap>
	Example:
	psbimap map - Prints sb imap
end

define psblmap
	if $argc == 0
		help psblmap
	else
		set $lmap = $arg0
		set $hash = 0
		set $hash_end = $lmap->iv_buckets
		printf "psblmap=%p, name=%s, count=%d, buckets=%d\n", $lmap, $lmap->ia_map_name, $lmap->iv_count, $hash_end
		while $hash < $hash_end
			set $node = $lmap->ipp_HT[$hash]
			set $i = 0
			while $node != 0
				set $k = $node->iv_id.l
				printf "h=%d, c=%d: k=%ld(0x%lx)\n", $hash, $i, $k, $k
				set $node = $node->ip_next
				set $i++
			end
			set $hash++
		end
	end
end

document psblmap
	Prints lmap (long map)
	Syntax: psblmap <lmap>
	Example:
	psblmap map - Prints sb lmap
end

define psbllmap
	if $argc == 0
		help psbllmap
	else
		set $llmap = $arg0
		set $hash = 0
		set $hash_end = $llmap->iv_buckets
		printf "psbllmap=%p, name=%s, count=%d, buckets=%d\n", $llmap, $llmap->ia_map_name, $llmap->iv_count, $hash_end
		while $hash < $hash_end
			set $node = $llmap->ipp_HT[$hash]
			set $i = 0
			while $node != 0
				set $k = $node->iv_id.ll
				printf "h=%d, c=%d: k=%lld(0x%llx)\n", $hash, $i, $k, $k
				set $node = $node->ip_next
				set $i++
			end
			set $hash++
		end
	end
end

document psbllmap
	Prints llmap (long long map)
	Syntax: psbllmap <llmap>
	Example:
	psbllmap map - Prints sb llmap
end

define psblocio
	set $locio = gp_local_mon_io
	printf "psblocio - gp_local_mon_io=%p\n", $locio
	if $locio != 0
		set $max = $locio->iv_client_buffers_max
		set $cshm = $locio->ip_cshm
		printf "basics\n"
		printf "  iv_nid=%d/iv_pid=%d (nid/pid)\n", $locio->iv_nid, $locio->iv_pid
		printf "  iv_qid=%d (msqid)\n", $locio->iv_qid
		printf "  iv_cmid=%d, ip_cshm=%p (shmid/addr)\n", $locio->iv_cmid, $cshm
		printf "  iv_mpid=%d (monitor pid)\n", $locio->iv_mpid
		printf "  iv_client_buffers_max=%d\n", $max
		printf "  iv_acquired_buffer_count=%d/max=%d\n", $locio->iv_acquired_buffer_count, $locio->iv_acquired_buffer_count_max
		printf "callbacks\n"
		printf "  event-cb="
		p $locio->ip_event_cb
		printf "  notice-cb="
		p $locio->ip_notice_cb
		printf "  recv-cb="
		p $locio->ip_recv_cb
		printf "  unsol-cb="
		p $locio->ip_unsol_cb
		# disabled for now!
		#printf "shm buffers\n"
		set $inx = 0
		set $sizememhdr = sizeof(struct LioSharedMemHdr)
		set $sizemsgdef = sizeof(SharedMsgDef)
		set $maxcount = $locio->iv_acquired_buffer_count
		set $cshm += $sizememhdr
		set $shm = (SharedMsgDef *) $cshm
		set $inx = $max
		while $inx < $max
			if $shm->trailer.bufInUse
				set $inuse = $shm->trailer.bufInUse
				set $msgtype = $shm->msg.type
				if $msgtype == MsgType_Service
					set $reptype = $shm->msg.u.reply.type
					if $reptype >= ReplyType_Generic
						set $reptypeinx = $reptype
						if $reptypeinx >= ReplyType_Invalid
							$reptypeinx = ReplyType_Invalid
						end
						set $reptypeinx -= ReplyType_Generic
						set $reptypestr = $locio->replyTypes_[$reptypeinx]
						printf "  buffer #%d (%p), reply=%d (%s), owner=%d\n", $inx, $shm, $reptype, $reptypestr, $inuse
					else
						set $reqtype = $shm->msg.u.request.type
						set $reqtypeinx = $reqtype
						if $reqtypeinx >= ReplyType_Generic
							$reqtypeinx = ReplyType_Invalid
						end
						set $reqtypestr = $locio->reqTypes_[$reqtypeinx]
						printf "  buffer #%d (%p), request=%d (%s), owner=%d\n", $inx, $shm, $reqtype, $reqtypestr, $inuse
					end
				else
					set $msgtypeinx = $msgtype
					if $msgtypeinx >= MsgType_Invalid
						$msgtypeinx = MsgType_Invalid
					end
					set $msgtypestr = $locio->msgTypes_[$msgtypeinx]
					printf "  buffer #%d (%p), message=%d (%s), owner=%d\n", $inx, $shm, $msgtype, $msgtypestr, $inuse
				end
			end
			set $inx++
			set $shm = (SharedMsgDef *) ($cshm+$sizemsgdef)
		end
	end
	printf "\n"
end

define psbmd
	if $argc == 0
		help psbmd
	else
		set $msg = $arg0
		if $argc > 1
			set $msghi = $arg1
		else
			set $msghi = $arg0
		end
		set $cap = SB_Trans::Msg_Mgr::cv_md_table.iv_cap
		set $table = SB_Trans::Msg_Mgr::cv_md_table.ipp_table
		if $msghi >= $cap
			printf "msgid-hi (%d) is invalid (bigger than cap=%d)\n", $msghi, $cap
			set $msghi = $cap - 1
		end
		if $msg < 0
			printf "msgid-lo (%d) is invalid (negative)\n", $msg
			set $msg = $msghi + 1
		end
		if $msg >= $cap
			printf "msgid-lo (%d) is invalid (bigger than cap=%d)\n", $msg, $cap
			set $msg = $msghi + 1
		end
		if $msghi < 0
			printf "msgid-hi (%d) is invalid (negative)\n", $msghi
			set $msg = $msghi + 1
		end
		while $msg <= $msghi
			printf "msgid=%d\n", $msg
			p *$table[$msg]
			set $msg++
		end
	end
end

document psbmd
	Prints md
	Syntax: psbmd <msgid-lo> [ <msgid-hi> ]
	Example:
	psbmd 1   - Prints sb md 1
	psbmd 2 4 - Prints sb md's 2-4
end

define psbmdall
	set $odelta = 0
	set $ots = 0
	if $argc >= 1
		set $ots = 1
	end
	if $argc >= 2
		set $odelta = 1
	end
	set $inx = -1
	set $cap = SB_Trans::Msg_Mgr::cv_md_table.iv_cap
	set $inuse = SB_Trans::Msg_Mgr::cv_md_table.iv_inuse
	printf "psbmdall - SB_Trans::Msg_Mgr::cv_md_table: cap=%d, inuse=%d\n", $cap, $inuse
	while ++$inx < $cap
		set $md = SB_Trans::Msg_Mgr::cv_md_table.ipp_table[$inx]
		set $stream = ('SB_Trans::Sock_Stream' *) $md->ip_stream
		printf "md[%d]=%p, inuse=%d, stream=%p ", $inx, $md, $md->iv_inuse, $stream
		if $stream == 0
			printf "?"
		else
			printf "%s", $stream->ia_stream_name
		end
		printf ", where="
		if $md->ip_where == 0
			printf "?"
		else
			printf "%s", $md->ip_where
		end
		if $ots == 0
			psbaintmdfmtslot
		else
			psbaintmdfmtts
		end
	end
	printf "\n"
end

document psbmdall
	Prints all mds
	Syntax: psbmdall [ timestamp [ delta ] ]
	Example:
	psbmdall     - Prints sb all mds
	psbmdall 1   - Prints sb all mds with timestamp
	psbmdall 1 1 - Prints sb all mds with timestamp/delta
end

define psbmdmap
	if $argc == 0
		help psbmdmap
	else
		set $mdmap = $arg0
		set $hash = 0
		set $hash_end = $mdmap->iv_buckets
		printf "psbmdmap=%p, name=%s, count=%d, buckets=%d\n", $mdmap, $mdmap->ia_md_map_name, $mdmap->iv_count, $hash_end
		while $hash < $hash_end
			set $node = $mdmap->ipp_HT[$hash]
			set $i = 0
			while $node != 0
				printf "h=%d, c=%d: msgid=%d\n", $hash, $i, $node->iv_id.i
				set $node = $node->ip_next
				set $i++
			end
			set $hash++
		end
	end
end

document psbmdmap
	Prints mdmap
	Syntax: psbmdmap <mdmap>
	Example:
	psbmdmap map - Prints sb mdmap
end

define psbmds
	set $odelta = 0
	set $ots = 0
	if $argc >= 1
		set $ots = 1
	end
	if $argc >= 2
		set $odelta = 1
	end
	set $count = 0
	set $inx = -1
	set $cap = SB_Trans::Msg_Mgr::cv_md_table.iv_cap
	set $inuse = SB_Trans::Msg_Mgr::cv_md_table.iv_inuse
	printf "psbmds - SB_Trans::Msg_Mgr::cv_md_table: cap=%d, inuse=%d\n", $cap, $inuse
	while ++$inx < $cap
		set $md = SB_Trans::Msg_Mgr::cv_md_table.ipp_table[$inx]
		if $md->iv_inuse != 0
			set $stream = ('SB_Trans::Sock_Stream' *) $md->ip_stream
			printf "md[%d]=%p, stream=%p ", $inx, $md, $stream
			if $stream == 0
				printf "?"
			else
				printf "%s", $stream->ia_stream_name
			end
			printf ", where="
			if $md->ip_where == 0
				printf "?"
			else
				printf "%s", $md->ip_where
			end
			if $ots == 0
				psbaintmdfmtslot
			else
				psbaintmdfmtts
			end
			set $count++
			if $count >= $inuse
				set $inx = $cap
			end
		end
	end
	printf "\n"
end

document psbmds
	Prints mds
	Syntax: psbmds [ timestamp [ delta ] ]
	Example:
	psbmds     - Prints sb mds
	psbmds 1   - Prints sb mds with timestamp
	psbmds 1 1 - Prints sb mds with timestamp/delta
end

define psbmsctrl
	printf "psbmsctrl\n"
	if $argc == 0
		help psbmsctrl
	else
		set $addr = $arg0
		printf "addr=%p\n", $addr
		set $mh = (message_header_template *) $addr
		set $d = $mh->dialect_type
		if $d == DIALECT_AMP_AMP_SQ
			set $map = &gv_ms_mh_req_amp_amp_sq_type_label_map
		else
			if $d == DIALECT_DP2_DP2
				set $map = &gv_ms_mh_req_dp2_dp2_type_label_map
			else
				if $d == DIALECT_DP2UTIL_DP2
			        	set $map = &gv_ms_mh_req_dp2util_dp2_type_label_map
				else
					if $d == DIALECT_FS_FS
				       		set $map = &gv_ms_mh_req_fs_fs_type_label_map
					else
						if $d == DIALECT_FS_IOPALL
				        		set $map = &gv_ms_mh_req_fs_iopall_type_label_map
						else
							if $d == DIALECT_FS_IOPDISK
					        		set $map = &gv_ms_mh_req_fs_iopdisk_type_label_map
							else
								if $d == DIALECT_FS2_IOPDISK
						        		set $map = &gv_ms_mh_req_fs2_iopdisk_type_label_map
								else
									if $d == DIALECT_RCV_IOPDISK
										set $map = &gv_ms_mh_req_rcv_iopdisk_type_label_map
									else
										if $d == DIALECT_TMF_IOPDISK
								        		set $map = &gv_ms_mh_req_tmf_iopdisk_type_label_map
										else
									        	set $map = 0
										end
									end
								end
							end
						end
					end
				end
			end
		end
		psbaintlabelget gv_ms_mh_dialect_type_label_map $d
		set $d_label = $label
		if $d == DIALECT_ZERO
			set $d0 = (struct dialect_zero_template *) $addr
			if $d0->p1 & 0xc000
				set $p1 = $d0->p1 & 0x3fff
				psbaintlabelget gv_ms_mh_p1_zero_type_label_map $p1
				printf "dtype=%d(%s), p1=%d(0x%x)(%s), p2=%d(0x%x), p3=%d(0x%x), p4=%d(0x%x), p6=%d(0x%x), p6=%d(0x%x)\n", $d, $d_label, $d0->p1, $d0->p1, $label, $d0->p2, $d0->p2, $d0->p3, $d0->p3, $d0->p4, $d0->p4, $d0->p5, $d0->p5, $d0->p6, $d0->p6
			else
				printf "dtype=%d(%s), p1=%d(0x%x), p2=%d(0x%x), p3=%d(0x%x), p4=%d(0x%x), p5=%d(0x%x), p6=%d(0x%x)\n", $d, $d_label, $d0->p1, $d0->p1, $d0->p2, $d0->p2, $d0->p3, $d0->p3, $d0->p4, $d0->p4, $d0->p5, $d0->p5, $d0->p6, $d0->p6
			end
		else
			if $map == 0
				printf "dtype=%d(%s), rtype=%d, rvers=%d, err/min-vers=%d\n", $d, $d_label, $mh->request_type, $mh->request_version, $mh->minimum_interpretation_version
			else
				psbaintlabelget $map $mh->request_type
				printf "dtype=%d(%s), rtype=%d(%s), rvers=%d, err/min-vers=%d\n", $d, $d_label, $mh->request_type, $label, $mh->request_version, $mh->minimum_interpretation_version
			end
		end
	end
end

document psbmsctrl
	Prints ms-ctrl
	Syntax: psbmsctrl <addr>
	Example:
	psbmsctrl addr - Prints sb ms-control
end

define psbmsopens
	set $cap = gv_ms_od_mgr.iv_cap
	set $inx = 0
	printf "psbmsopens - gv_ms_od_mgr: cap=%d\n", $cap
	while $inx < $cap
		set $od = gv_ms_od_mgr.ipp_table[$inx]
		if $od && $od->iv_inuse
			if $od->iv_self
				printf "od[%d]=%p, self\n", $inx, $od
			else
				if $od->ip_stream
					printf "od[%d]=%p, pname=%s, prog=%s, p-id=%d/%d\n", $inx, $od, $od->ia_process_name, $od->ia_prog, $od->iv_nid, $od->iv_pid
				else
					printf "od[%d]=%p, inprogress\n", $inx, $od
				end
			end
		end
		set $inx++
	end
	printf "\n"
end

define psbnidpidmap
	set $npmap = &gv_sb_stream_nidpid_map
	set $hash = 0
	set $hash_end = $npmap->iv_buckets
	printf "psbnidpidmap - count=%d\n", $npmap->iv_count
	while $hash < $hash_end
		set $node = $npmap->ipp_HT[$hash]
		set $i = 0
		while $node != 0
			set $npnode = (NPS_Node *) $node
			set $np = (NidPid_Type *) &$npnode->iv_link.iv_id.ll
			set $stream = $npnode->ip_stream
			printf "h=%d, c=%d: p-id=%d/%d-stream=%p (%s)\n", $hash, $i, $np->u.i.iv_nid, $np->u.i.iv_pid, $stream, $stream->ia_stream_name
			set $node = $node->ip_next
			set $i++
		end
		set $hash++
	end
end

define psbod
	if $argc == 0
		help psbod
	else
		set $oid = $arg0
		set $cap = gv_ms_od_mgr.iv_cap
		set $table = gv_ms_od_mgr.ipp_table
		if $oid >= $cap
			printf "oid (%d) is invalid (bigger than cap=%d)\n", $oid, $cap
			set $oid = $cap
		end
		if $oid < 0
			printf "oid (%d) is invalid (negative)\n", $oid
			set $oid = $cap
		end
		if $oid != $cap
			printf "oid=%d\n", $oid
			if $table[$oid] == 0
				printf "od is NULL\n"
			else
				p *$table[$oid]
			end
		end
	end
end

document psbod
	Prints od
	Syntax: psbod <oid>
	Example:
	psbod 1   - Prints sb od 1
end

define psbover
	printf "psbover -\n"
	psbid
	psbenvvars
	psbassert
	psbthreads
	psbevents
	psbrecvq
	psbcompq
	psbtimers
	psbmsopens
	psbstreamdelq
	psbstreams
	psbmds
	psblocio
end

define psboverwfs
	printf "psboverwfs -\n"
	psbid
	psbenvvars
	psbassert
	psbthreads
	psbevents
	psbrecvq
	psbcompq
	psbtimers
	psbfsopens
	psbmsopens
	psbstreamdelq
	psbstreams
	psbmds
	psblocio
end

define psbrecvq
	set $recvq = &gv_ms_recv_q
	printf "psbrecvq - gv_ms_recv_q=%p, name=%s, qid=%d, count=%d\n", $recvq, $recvq->ia_d_q_name, $recvq->iv_qid, $recvq->iv_count
	set $inx = 0
	set $max = $recvq->iv_count
	set $dql = $recvq->ip_head
	while $inx < $max
		set $md = (MS_Md_Type *) $dql
		set $out = $md->out
		set $mdtype = $out.iv_msg_type
		psbaintlabelget gv_sb_md_type_label_map $mdtype
		set $mdtypelabel = $label
		printf "md[%d]=%p, qid=%d, msgid=%d, fserr=%d, type=%d(%s), p-id=%d/%d, mon-msg=%d, ctrl=%p/%d, data=%p/%d\n", $inx, $dql, $dql->iv_qid, $dql->iv_id.i, $out.iv_fserr, $mdtype, $mdtypelabel, $out.iv_nid, $out.iv_pid, $out.iv_mon_msg, $out.ip_recv_ctrl, $out.iv_recv_ctrl_size, $out.ip_recv_data, $out.iv_recv_data_size
		set $dql = $dql->ip_next
		set $inx++
	end
	printf "\n"
end

define psbrmmap
	if $argc == 0
		help psbrmmap
	else
		set $rmmap = $arg0
		set $hash = 0
		set $hash_end = $rmmap->iv_buckets
		printf "psbrmmap=%p, name=%s, count=%d, buckets=%d\n", $rmmap, $rmmap->ia_map_name, $rmmap->iv_count, $hash_end
		while $hash < $hash_end
			set $node = $rmmap->ipp_HT[$hash]
			set $i = 0
			while $node != 0
				set $rmnode = (RM_Node *) $node
				printf "h=%d, c=%d: reqid=%d/msgid=%d\n", $hash, $i, $rmnode->iv_link.iv_id.i, $rmnode->iv_msgid
				set $node = $node->ip_next
				set $i++
			end
			set $hash++
		end
	end
end

document psbrmmap
	Prints rmmap (reqid msgid map)
	Syntax: psbrmmap <rmmap>
	Example:
	psbrmmap map - Prints sb rmmap
end

define psbslotmgr
	if $argc == 0
		help psbslotmgr
	else
		set $slotmgr = $arg0
		set $inx = 0
		set $count = 0
		set $cap = $slotmgr->iv_cap
		set $max = $slotmgr->iv_max
		printf "psbslotmgr - alloc=%d, cap=%d, free=%d, max=%d\n", $slotmgr->iv_alloc, $slotmgr->iv_cap, $slotmgr->iv_free, $slotmgr->iv_max
		printf "head=%d, tail=%d\n", $slotmgr->iv_head, $slotmgr->iv_tail
		printf "slots=%p\n", $slotmgr->ip_slots
		while $inx < $cap
			set $next = $slotmgr->ip_slots[$inx]
			if $next == -2
				printf "slots[%d]=%d\n", $inx, $next
				set $count++
				if $inx >= $max
					set $inx = $cap
				end
			end
			set $inx++
		end
	end
end

document psbslotmgr
	Prints slotmgr - slots
	Syntax: psbslotmgr <slotmgr>
	Example:
	psbslotmgr slotmgr - Prints sb slotmgr
end

define psbsmap
	if $argc == 0
		help psbsmap
	else
		set $smap = $arg0
		set $hash = 0
		set $hash_end = $smap->iv_buckets
		printf "psbsmap=%p, name=%s, count=%d, buckets=%d\n", $smap, $smap->ia_map_name, $smap->iv_count, $hash_end
		while $hash < $hash_end
			set $node = $smap->ipp_HT[$hash]
			set $i = 0
			while $node != 0
				if $node->iv_use_vvalue
					printf "h=%d, c=%d: k=%s, vv=%s\n", $hash, $i, $node->ip_key, $node->ip_vvalue
				else
					printf "h=%d, c=%d: k=%s, v=%s\n", $hash, $i, $node->ip_key, $node->ip_value
				end
				set $node = ('SB_Smap::SML_Type' *) $node->iv_link.ip_next
				set $i++
			end
			set $hash++
		end
	end
end

document psbsmap
	Prints smap (string map)
	Syntax: psbsmap <smap>
	Example:
	psbsmap map - Prints sb smap
end

define psbstreamdelq
	set $inx = 0
	set $delq = 'SB_Trans::Trans_Stream'::cv_del_q
	set $count = $delq.iv_count
	set $link = $delq.ip_head
	set $stream = ('SB_Trans::Trans_Stream' *) $link
	set $off = (char *) &$stream->iv_del_link - (char *) $link
	printf "psbstreamdelq - SB_Trans::Trans_Stream::cv_del_q: count=%d, head=%p, tail=%p\n", $count, $delq.ip_head, $delq.ip_tail
	while $inx < $count
		set $stream = ('SB_Trans::Trans_Stream' *) ((char *) $link - $off)
		printf "q[%d] link=%p, stream=%p (%s), ref=%d\n", $inx, $link, $stream, $stream->ia_stream_name, $stream->iv_md_ref_count.iv_val
		set $link = $stream->iv_del_link.ip_next
		set $inx++
	end
	printf "\n"
end

define psbstreams
	set $acc_count = SB_Trans::Trans_Stream::cv_stream_acc_count.iv_val
	set $con_count = SB_Trans::Trans_Stream::cv_stream_con_count.iv_val
	set $total_count = SB_Trans::Trans_Stream::cv_stream_total_count.iv_val
	set $acc_hi_count = SB_Trans::Trans_Stream::cv_stream_acc_hi_count.iv_val
	set $con_hi_count = SB_Trans::Trans_Stream::cv_stream_con_hi_count.iv_val
	set $total_hi_count = SB_Trans::Trans_Stream::cv_stream_total_hi_count.iv_val
	printf "psbstreams - SB_Trans::Trans_Stream\n"
	printf "stream counts total=%d, con=%d, acc=%d\n", $total_count, $con_count, $acc_count
	printf "stream counts hi-total=%d, hi-con=%d, hi-acc=%d\n", $total_hi_count, $con_hi_count, $acc_hi_count
	set $smap = SB_Trans::Sock_Stream::cv_stream_map
	set $hash = 0
	set $hash_end = $smap.iv_buckets
	while $hash < $hash_end
		set $link = (SS_Node *) $smap.ipp_HT[$hash]
		set $i = 0
		while $link != 0
			set $stream = $link->ip_stream
			printf "hash=%d, i=%d, stream=%p, name=%s\n", $hash, $i, $link, $stream->ia_stream_name
			set $link = (SS_Node *) $link->iv_link.ip_next
			set $i++
		end
		set $hash++
	end
	printf "\n"
end

define psbtable
	if $argc == 0
		help psbtable
	else
		set $table = $arg0
		set $inx = 0
		set $count = 0
		set $cap = $table->iv_cap
		set $inuse = $table->iv_inuse
		printf "psbtable - inuse=%d\n", $table->iv_inuse
		while $inx < $cap
			set $entry = $table->ipp_table[$inx]
			if $entry
				set $count++
				printf "inx=%d, count=%d\n", $inx, $count
				p *$entry
				if $count >= $inuse
					set $inx = $cap
				end
			end
			set $inx++
		end
	end
end

document psbtable
	Prints table (table)
	Syntax: psbtable <table>
	Example:
	psbtable table - Prints sb table
end

define psbthreads
	set $cap = gv_sb_thread_table.iv_cap
	set $inuse = gv_sb_thread_table.iv_inuse
	set $count = 0
	set $inx = 0
	set $timer_tls_inx = gv_timer_tls_inx
	printf "psbthreads - gv_sb_thread_table: cap=%d, inuse=%d\n", $cap, $inuse
	while $inx < $cap
		set $thread = gv_sb_thread_table.ipp_table[$inx]
		if $thread != 0
			set $self = $thread->iv_self
			printf "thread[%d]=%p, tid=%d, name=%s, self=%p\n", $inx, $thread, $thread->iv_tid, $thread->ia_thread_name, $self
			psbaintthreadstls $self
		end
		set $count++
		if $count >= $inuse
			set $inx = $cap
		end
		set $inx++
	end
	printf "\n"
end

define psbtimermap
	if $argc == 0
		help psbtimermap
	else
		set $tmap = $arg0
		set $hash = 0
		set $hash_end = $tmap->iv_buckets
		printf "psbtimermap=%p, name=%s, count=%d, buckets=%d, last-time-check=%lld\n", $tmap, $tmap->ia_map_name, $tmap->iv_count, $hash_end, $tmap->iv_time_last_check.iv_tics
		while $hash < $hash_end
			set $link = $tmap->ipp_HT[$hash]
			set $i = 0
			while $link != 0
				printf "h=%d, c=%d: link=%p, next=%p, prev=%p, item=%p, user-param=%ld(0x%lx), tics=%d, pop-time=%lld\n", $hash, $i, $link, $link->ip_next, $link->ip_prev, $link->ip_item, $link->iv_user_param, $link->iv_user_param, $link->iv_tics, (long long) $link->iv_pop_time.iv_tics
				set $link = $link->ip_next
				set $i++
			end
			set $hash++
		end
	end
end

document psbtimermap
	Prints timermap (timer map)
	Syntax: psbtimermap <time>
	Example:
	psbtimermap map - Prints sb timermap
end

define psbtimer
	if $argc == 0
		help psbtimer
	else
		set $cap = gp_timer_tle_mgr->iv_cap
		set $tinx = $arg0
		printf "psbtimer %d\n", $tinx
		if $tinx < 0
			printf "invalid timer\n"
			set $tinx = $cap
		end
		if $tinx >= $cap
			printf "invalid timer\n"
		end
		if $tinx < $cap
			set $tle = &gp_timer_tles[$tinx]
			psbainttle 0
		end
	end
end

document psbtimer
	Prints timer
	Syntax: psbtimer <timer>
	Example:
	psbtimer 1 - Prints sb timer 1
end

define psbtimers
	set $tinx = 0
	set $tle = gp_timer_head
	if gp_timer_tle_mgr == 0
		set $cap = 0
		set $free = 0
	else
		set $cap = gp_timer_tle_mgr->iv_cap
		set $free = gp_timer_tle_mgr->iv_free
	end
	printf "psbtimers - gv_timer_head=%p, tle-cap=%d, tle-free=%d\n", $tle, $cap, $free
	while $tle != 0
		psbainttle 0
		set $tinx++
		set $tle = $tle->ip_next
	end
	if $cap > 0
		printf "tles\n"
	end
	set $tinx = 1
	while $tinx < $cap
		set $tle = &gp_timer_tles[$tinx]
		if $tle->iv_inuse != 0
			psbainttle 0
		end
		set $tinx++
	end
	printf "\n"
end

define psbtracemem
	if $argc == 0
		set $cnt = 20
	else
		set $cnt = $arg0
	end
	set $tinx = gv_trace.iv_trace_mem_inx
	set $tsize = gv_trace.iv_trace_mem_size
	set $tbuf = gv_trace.ip_trace_mem_buf
	printf "psbtracemem - gv_trace - inx=%d, size=%d\n", $tinx, $tsize
	set $inx = 0
	set $reccnt = 0
	while $inx < $tinx
		if $tbuf[$inx] == '\n'
			set $reccnt++
		end
		set $inx++
	end
	if $cnt < $reccnt
		set $startrec = $reccnt - $cnt
	else
		set $startrec = 0
	end
	set $inx = 0
	set $start = $inx
	set $rec = 0
	while $inx < $tinx
		if $tbuf[$inx] == '\n'
			if $rec >= $startrec
				set $size = $inx - $start
				set print elements $size
				p &$tbuf[$start]
			end
			set $inx++
			set $start = $inx
			set $rec++
		else
			set $inx++
		end
	end
	set print elements 200
	printf "\n"
end

define psbutraceapi
	if $argc == 0
		set $cnt = 20
	else
		set $cnt = $arg0
	end
	set $inx = sb_utrace_api.iv_inx
	set $max = sb_utrace_api.iv_max
	set $wrap = sb_utrace_api.iv_wrapped
	if $cnt > $max
		set $cnt = $max
	end
	printf "psbutraceapi - sb_utrace_api - inx=%d, max=%d, wrapped=%d\n", $inx, $max, $wrap
	if $wrap != 0
		set $wcnt = $cnt - $inx
		set $winx = $max - $wcnt
		if $wcnt < 0
			set $cntr = $wcnt
		else
			set $cntr = 0
			set $cnt -= $wcnt
		end
		while $cntr < $wcnt
			set $rec = sb_utrace_api.ip_buf[$winx]
			psbaintutraceapirec $winx $rec
			set $winx++
			set $cntr++
		end
	end
	if $inx < $cnt
		set $cnt = $inx
	end
	set $inx -= $cnt
	set $cntr = 0
	while $cntr < $cnt
		set $rec = sb_utrace_api.ip_buf[$inx]
		psbaintutraceapirec $inx $rec
		set $inx++
		set $cntr++
	end
	printf "\n"
end

define psbutracempi
	if $argc == 0
		set $cnt = 20
	else
		set $cnt = $arg0
	end
	set $inx = sb_utrace_mpi.iv_inx
	set $max = sb_utrace_mpi.iv_max
	set $wrap = sb_utrace_mpi.iv_wrapped
	if $cnt > $max
		set $cnt = $max
	end
	printf "psbutracempi - sb_utrace_mpi - inx=%d, max=%d, wrapped=%d\n", $inx, $max, $wrap
	if $wrap != 0
		set $wcnt = $cnt - $inx
		set $winx = $max - $wcnt
		if $wcnt < 0
			set $cntr = $wcnt
		else
			set $cntr = 0
			set $cnt -= $wcnt
		end
		while $cntr < $wcnt
			set $rec = sb_utrace_mpi.ip_buf[$winx]
			psbaintutracempirec $winx $rec
			set $winx++
			set $cntr++
		end
	end
	if $inx < $cnt
		set $cnt = $inx
	end
	set $inx -= $cnt
	set $cntr = 0
	while $cntr < $cnt
		set $rec = sb_utrace_mpi.ip_buf[$inx]
		psbaintutracempirec $inx $rec
		set $inx++
		set $cntr++
	end
	printf "\n"
end

define psbutraceprof
	if $argc == 0
		set $cnt = 20
	else
		set $cnt = $arg0
	end
	set $inx = gv_sb_test_profiler_trace.iv_inx
	set $max = gv_sb_test_profiler_trace.iv_max
	set $wrap = gv_sb_test_profiler_trace.iv_wrapped
	set $rnddelay = gv_sb_test_profiler_rnddelay
	set $samplesec = gv_sb_test_profiler.iv_sample.it_value.tv_sec
	set $sampleusec = gv_sb_test_profiler.iv_sample.it_value.tv_usec
	if $cnt > $max
		set $cnt = $max
	end
	printf "psbutraceprof - gv_sb_test_profiler_trace - inx=%d, max=%d, wrapped=%d, rnddelay=%f, sample=%d.%09d\n", $inx, $max, $wrap, $rnddelay, $samplesec, $sampleusec
	if $wrap != 0
		set $wcnt = $cnt - $inx
		set $winx = $max - $wcnt
		if $wcnt < 0
			set $cntr = $wcnt
		else
			set $cntr = 0
			set $cnt -= $wcnt
		end
		while $cntr < $wcnt
			set $rec = gv_sb_test_profiler_trace.ip_buf[$winx]
			printf "rec[%05d]: time=%d.%06d, to=%d, delta=%d\n", $winx, $rec.iv_time.tv_sec, $rec.iv_time.tv_usec, $rec.iv_to, $rec.iv_delta
			set $winx++
			set $cntr++
		end
	end
	if $inx < $cnt
		set $cnt = $inx
	end
	set $inx -= $cnt
	set $cntr = 0
	while $cntr < $cnt
		set $rec = gv_sb_test_profiler_trace.ip_buf[$inx]
		printf "rec[%05d]: time=%d.%06d, to=%d, delta=%d\n", $inx, $rec.iv_time.tv_sec, $rec.iv_time.tv_usec, $rec.iv_to, $rec.iv_delta
		set $inx++
		set $cntr++
	end
	printf "\n"
end

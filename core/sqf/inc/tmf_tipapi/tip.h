// @@@ START COPYRIGHT @@@
//
// Licensed to the Apache Software Foundation (ASF) under one
// or more contributor license agreements.  See the NOTICE file
// distributed with this work for additional information
// regarding copyright ownership.  The ASF licenses this file
// to you under the Apache License, Version 2.0 (the
// "License"); you may not use this file except in compliance
// with the License.  You may obtain a copy of the License at
//
//   http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing,
// software distributed under the License is distributed on an
// "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied.  See the License for the
// specific language governing permissions and limitations
// under the License.
//
// @@@ END COPYRIGHT @@@

#ifndef  tip_h
#define  tip_h

/*
 * TIP constants
 */

enum Tip_MessageTypes { 
		TIPMSG_PUSH		= 1,  
		TIPMSG_PULL		= 2,
		TIPMSG_PULL_ASYNC	= 3,
		TIPMSG_PULL_COMPLETE	= 4,
		TIPMSG_OPEN		= 5,
		TIPMSG_XID_TO_URL	= 6
};
/*
 * TIP external datatypes
 */
typedef unsigned int		tip_handle_t;

#ifdef __cplusplus
extern "C" {
#endif

/*
 * tip_open establishes a connection to the tip gateway. The environment 
 * variable TIPGATEWAY points to a well known process.
 */
int tip_open( 
				 /* [out] */ tip_handle_t *pgateway
				);


/*
 * tip_close closes connection to tip gateway
 */
int tip_close(
				  /* [in] */ tip_handle_t gateway
				 );


/*
 * tip_resume asssociates calling process with TMF transaction.
 * Sets current transaction of calling process.
 */
int tip_resume( 
					/* [in] */  void        *plocal_xid
				  );


/*
 * tip_suspend breaks association of calling process with transaction
 */
int tip_suspend(
					 /* [out] */ void		*plocal_xid,
					 /* [in]  */ unsigned int	length
					);


/*
 * tip_xid_to_url returns a TIP URL for a TMF transaction identifier
 */
int tip_xid_to_url(
						 /* [in]  */ void		*plocal_xid,
						 /* [out] */ char		*pxid_url,
						 /* [in]  */ unsigned int	url_length
						);

/*
 * tip_xid_to_url_nw returns a TIP URL for a TMF transaction identifier.
 * The call returns immediately after the request is made.  If completion
 * is immediate, then completed is set non-zero.  Otherwise, completion
 * occurs when AWAITIOX completes and tip_interpret_xid_to_url is called.
 */
int tip_xid_to_url_nw(   
					  /* [in] */	tip_handle_t gateway,
					  /* [in] */	char		*pbuffer,
					  /* [in] */	void		*plocal_xid,
					  /* [out] */	char		*pxid_url,
					  /* [in] */	unsigned int	url_length,
					  /* [in] */	unsigned int	tag,
					  /* [out] */	short		*pfilenum,
					  /* [out] */	int		*completed
					  );

/*
 * tip_url_to_xid translates a TIP URL into a TMF transaction identifier 
 */
int tip_url_to_xid(
						 /* [in]  */ char		*pxid_url,
						 /* [out] */ void		*plocal_xid,
						 /* [in]  */ unsigned int	xid_length
						);


/*
 * tip_push exports the current transaction to a remote node and returns
 * a TIP URL for the associated remote transaction
 */
int tip_push(
				 /* [in]  */ tip_handle_t	gateway,
				 /* [in]  */ char		*pendpoint_url,
				 /* [in]  */ void		*plocal_xid,
				 /* [out] */ char		*pxid_url,
				 /* [in]  */ unsigned int	url_length
				);


/* 
 * tip_push_nw exports the current transaction to a remote node.
 * The call returns immediately after the request is made.  If completion
 * is immediate, then completed is set non-zero.  Otherwise, completion
 * occurs when AWAITIOX completes and tip_interpret_push is called.
 */
int tip_push_nw (	
					/* [in] */ tip_handle_t gateway,
					/* [in] */ char		*pbuffer,
					/* [in] */ char		*ptm_url,
					/* [in] */ void         *plocal_xid,
					/* [out] */ char	*pxid_url,
					/* [in] */ unsigned int url_length,
					/* [in] */ unsigned int	tag,
					/* [out] */ short	*pfilenum,
					/* [out] */ int		*completed
					);


/*
 * tip_pull creates a local transaction and registers it with a remote transaction
 */
int tip_pull(
				 /* [in]  */ tip_handle_t	gateway,
				 /* [in]  */ char		*pxid_url,
				 /* [out] */ void		*plocal_xid,
				 /* [in]  */ unsigned int	xid_length
				);


/*
 * tip_pull_async creates a local transaction and registers it with a remote
 * transaction. The call returns immediately after the local transaction has been created.
 */
int tip_pull_async(
						 /* [in]  */ tip_handle_t	gateway,
						 /* [in]  */ char		*pxid_url,
						 /* [out] */ void		*plocal_xid,
						 /* [in]  */ unsigned int	xid_length,
						 /* [out] */ int		*completed 
						);


/*
 * tip_pull_nw creates a local transaction and registers it with a remote
 * transaction.  
 * The call returns immediately after the request is made.  If completion
 * is immediate, then completed is set non-zero.  Otherwise, completion
 * occurs when AWAITIOX completes and tip_interpret_pull is called.
 */
int tip_pull_nw(
				/* [in] */  tip_handle_t	gateway,
				/* [in] */  char		*pbuffer,
				/* [in] */  char		*pxid_url,    
				/* [out] */ void		*plocal_xid,
				/* [in] */  unsigned int	xid_length,
				/* [in] */  unsigned int	tag, 
				/* [out] */ short		*pfilenum,
				/* [out] */ int			*completed 
				);

/*
 * tip_pull_async_nw creates a local transaction and registers it with a remote
 * transaction. The call returns immediately after the local transaction  has
 * been created. 
 * The call returns immediately after the request is made.  If completion
 * is immediate, then completed is set non-zero.  Otherwise, completion
 * occurs when AWAITIOX completes and tip_interpret_pull_ is called.
 */

int tip_pull_async_nw (
					   /* [in] */ tip_handle_t  gateway,
					   /* [in] */ char          * buffer,
					   /* [in] */ char          *pxid_url,    
					   /* [out] */void          *plocal_xid,
					   /* [in] */ unsigned int  xid_length,
					   /* [in] */ unsigned int  tag,                 
					   /* [out] */short         *pfilenum,
					   /* [out] */int	    *completed 
					   );

/*
 * tip_pull_complete checks whether a local transaction has been successfully
 * registered with a remote transaction
 */
int tip_pull_complete(
						/* [in] */ tip_handle_t   gateway,
						/* [in] */ void           *plocal_xid 
							);	


/*
 * tip_pull_complete_nw checks whether a local transaction has been successfully
 * registered with a remote transaction
 */
int tip_pull_complete_nw(
							/* [in]  */ tip_handle_t	gateway,
							/* [in]  */ char		*pbuffer,
							/* [in]  */ void		*plocal_xid,
							/* [in]  */ unsigned int	tag,
							/* [out] */ short		*pfilenum,
							/* [out] */ int			*completed 
							);


/*
 * tip_interpret_push interprets tip_push_nw() response buffer
 */
int tip_interpret_push(
							  /* [in]  */ char           *pbuffer,
							  /* [out] */ char           *pxid_url,
							  /* [in]  */ unsigned int    xid_url_length
							 );


/*
 * tip_interpret_pull interprets tip_pull_nw() response buffer
 */
int tip_interpret_pull(
							  /* [in]  */ char           *pbuffer,
							  /* [out] */ void           *plocal_xid,
							  /* [in]  */ unsigned int    xid_length
							 );


/*
 * tip_interpret_pull_complete interprets tip_pull_complete_nw response buffer
 */
int tip_interpret_pull_complete(
								/* [in]  */ char           *pbuffer
								 );


/*
 * tip_interpret_pull_xid_to_url interprets tip_pull_xid_to_url_nw
 * response buffer.
 */
int tip_interpret_xid_to_url( 
							 /* [in] */ char			*pbuffer,
							 /* [out] */ char			*pxid_url,
							 /* [in] */ unsigned int	url_length
							 );                            

/*
 * tip_get_buffer_type returns reply buffer type
 */
int tip_get_buffer_type(
								/* [in]  */ char            *pbuffer,
								/* [out] */ unsigned int    *ptype
							  );

/*
 * Returns the length in bytes required for a tip buffer.    
 */
int tip_get_buffer_len ();

/*
 * Returns the length in bytes required for the local tm url.   
 */
int tip_get_tm_url_len();

/*
 * gets the TIP URL of the local transaction manager.
 */
int tip_get_tm_url (     
					  /* [in] */ tip_handle_t	gateway,
					  /* [out] */ char        *ptm_url,
					  /* [in] */ int		    tm_len
					  );                

/*
 * Returns the length in bytes required for a local url.   
 */
int tip_get_url_len();

#ifdef __cplusplus
}
#endif

#endif

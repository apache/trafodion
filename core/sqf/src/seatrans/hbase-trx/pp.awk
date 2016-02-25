#  The following example has been tested to work.
#  If the file f1 contains the following, run this script
#  as follows:    
#   awk -f pp.awk -v distro=HDP2.3 f1 > f2
#  contents of  file f1:
#  #ifdef HDP2.3
#  this is HDP2.3 specific code
#  #else
#  this is other than HDP2.3 code
#  #endif
#
#  #ifndef HDP2.3
#  this is other than HDP2.3 code partof ifndef
#  #else
#  this is HDP2.3 code part of ifndef else
#   #endif
#
#  this is common code
#  this is common code 2
#
#  #ifdef CDH1.0
#  this is CDH specific code
#  #endif

BEGIN{
printline=1      #print current line or not.
matchBegun=0     #match succeeded, keep printing
unmatchBegun=0   #opposite of  match, keep non-printing
ifdefpattern = "#ifdef"
endifpattern = "#endif"
elsepattern = "#else"
ifndefpattern = "#ifndef"
#distro passed in as argument
}
{
  #By default print everything
  printline =1

#########################
#  This section deals with resetting flags
########################
  #reset when #endif is encountered
  if(($0 ~ endifpattern) && (matchBegun == 1))
    {
      printline = 0
     matchBegun = 0
    }

  #reset when #endif is encountered
  if(($0 ~ endifpattern) && (unmatchBegun == 1))
    {
      printline = 0
     unmatchBegun = 0
    }
 
  #when #else is encountered and unmatchBegun, reset unmatchBegun
  #start print subsequent lines. 
  if(($0 ~ elsepattern) && (unmatchBegun == 1))
    {
      printline = 0
      unmatchBegun = 0
      matchBegun = 1
    }
  else
  {
    #when #else is encountered and matchBegun, reset matchBegun
    #start unprint subsequent lines by setting unmatchBegun 
    if(($0 ~ elsepattern) && (matchBegun == 1))
    {
      printline = 0
      unmatchBegun = 1
    }  
  }

##########################
# This section matches distro string and sets flags
##########################

  if($0 ~ ifdefpattern)
   {
     if( $2 ~ distro)
     {
       printline = 0
       matchBegun = 1
     }
     else
     {
      printline = 0
      unmatchBegun = 1
     }
   }
 if($0 ~ ifndefpattern)
  {
    if($2 ~ distro)
    {
      printline = 0
      unmatchBegun = 1
    }
    else
    {
     printline = 0
     matchBegun = 1
    }
  }
######################
# This section is final printing based on flags
######################

  #if unmatchBegun discard those lines 
  if(unmatchBegun == 1)
     printline = 0
 
  if(printline == 1)
	print $0;
}
END {
}

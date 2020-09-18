/* REXX */
/* 
 * Parse a JSON stream from z/OSMF for the sysvar call and print out the value of the passed in 'keyname'
 * See: http://tech.mikefulton.ca/WebEnablementToolkit for details on the REXX JSON parsing services
 */

trace 'o'
Parse Arg keyname .

  if (keyname = '' | keyname = '?') then do
    call SayErr 'Syntax: readsysvar <keyname>'
    call SayErr '  Where <keyname> is the keyname of the system variable returned in the system-variable-list'
    call SayErr '  The value of the keyname is written to stdout'
    return 4
  end

  rc = readJSON()
  if (rc <> 0) then do
    call SayErr 'readsysvar failed'
    return rc
  end
  do el = 1 to json.SYSTEM_VARIABLE_LIST.0
    entry = json.SYSTEM_VARIABLE_LIST.el.NAME
    if (entry = keyname) then do
      say json.SYSTEM_VARIABLE_LIST.el.VALUE
      return 0
    end
  end
  call SayErr 'Unable to find keyname: ' || keyname
  return 4

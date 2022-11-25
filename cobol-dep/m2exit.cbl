001876*-----------------------------------------------------------------
001877* :(copyright
001878* Copyright (c) 2022 IBM Canada Ltd.
001879* Authors Bernie Rataj <brataj@ca.ibm.com>
001880*         Mike Fulton <fultonm@ca.ibm.com>
      *
001881* This is a sample only, and is covered by the
001882* IBM Non-Warranted Sample Code License (ibm-nwsc)
001883* :)copyright
001890*A-1-B--+----2----+----3----+----4----+----5----+----6----+----7-|
001900 identification division.
002005 program-id. "MEXIT".
002012*
002013 environment division.
002014 configuration section.
002015 repository.
002016     function hex-of intrinsic.
002017 source-computer. zArchitecture
002020     .
002600 data division.
002800 working-storage section.
002901*
002906 77  exit-is-initial       pic s9(4) comp-5 value 1.
002907     88 EXIT_IS_INITIAL    value 1 false is ZERO.
002908 77  libexit-is-initial    pic s9(4) comp-5 value 1.
002909     88 LIBEXIT_IS_INITIAL value 1 false is ZERO.
002910
002911 77  copybook-reposition   pic s9(4) comp-5 value 0.
002912     88 COPYBOOK_REPOSITION    value 1 false is ZERO.
002913
002914 77  serial-count      pic s9(9) comp-5 value 0.
       77  dot-count         pic s9(9) comp-5 value 0.
       77  empty             pic x(30).
       77  raw-file          pic x(30).
002915 77  srcno             pic s9(4) comp-5.
002918*
002919* The copybook stack includes an entry for the outer level
002920* code (file-element(1)) and up to FILE_STACK_SIZE - 1 nested
002921* copies.
002922*
002923 77  copybook-level    pic s9(4) comp-5 value 1.
002924     88 COPYBOOK_INACTIVE  value 1.
002925 77  libno             redefines copybook-level *> abbreviation
002926                       pic s9(4) comp-5.
002927 77  FILE_STACK_SIZE   pic s9(4) comp-5 value 17.
002928 1   file-stack.
002930  2  file-element      occurs 17. *> FILE_STACK_SIZE
003000   3 file-ptr          pointer.
003001   3 file-count        pic s9(9) comp-5.
003003   3 file-library      pic x(8).
         3 file-member       pic x(32).
003020   3 file-buffer       pic x(80).
003100*
003110 local-storage section.
003126 77  filename          pic x(32).
003127 77  file-status       pic s9(9) comp-5.
003128 77  lib-ptr           pointer.
003129
003130 77  when-compiled-pic pic 9999/99/99B99,99,99.
003131 77  when-compiled-str redefines when-compiled-pic
003132                       pic x(19).
003140*
003200 linkage section.
003201 77  exit-type         pic 9(4) comp-5.
003202     88 EXIT-TYPE-INEXIT       VALUE 1.
003203     88 EXIT-TYPE-LIBEXIT      VALUE 2.
003204
003205 77  exit-operation    pic 9(4) comp-5.
003206     88 EXIT-OPERATION-OPEN    VALUE 0.
003207     88 EXIT-OPERATION-CLOSE   VALUE 1.
003208     88 EXIT-OPERATION-GET     VALUE 2.
003209     88 EXIT-OPERATION-FIND    VALUE 4.
003210
003211 77  exit-returncode   pic 9(9) comp-5.
003212     88 EXIT-RETURNCODE-OK     VALUE 0.
003213     88 EXIT-RETURNCODE-EOF    VALUE 4.
003214     88 EXIT-RETURNCODE-FAILED VALUE 12.
003215
003216 1   exit-work-area.
003217  2  exit-work-inexit  pointer.
003218  2  exit-work-libexit pointer.
003219  2  exit-work-prtexit pointer.
003220  2  exit-work-adexit  pointer.
003221  2  exit-work-rsvd    pointer.
003222  2  exit-work-msgexit pointer.
003223
003224 77  exit-data-length  pic 9(9) comp-5.
003225 77  exit-data-buffer  pointer.
003226
003227 77  exit-system-library   pic x(8).
003228 77  exit-system-member    pic x(8).
003229
003230 77  exit-library      pic x(30).
003231 77  exit-member       pic x(30).
003232
003233 1   parm.
003234  2  parm-length       pic s9(4) comp-5.
003235  2  parm-data         pic x(256).
003236
003238/*****************************************************************
003239* main
003240******************************************************************
003241 procedure division using
003243     exit-type             *>  1
003244     exit-operation        *>  2
003245     exit-returncode       *>  3
003246     exit-work-area        *>  4
003247     exit-data-length      *>  5
003248     exit-data-buffer      *>  6
003249     exit-system-library   *>  7
003250     exit-system-member    *>  8
003251     exit-library          *>  9
003252     exit-member           *> 10
003253     .
003254
003255     evaluate TRUE
003256
003257     when EXIT-TYPE-INEXIT
003259         perform exit-inexit
003260
003261     when EXIT-TYPE-LIBEXIT
003263         perform exit-libexit
003264
003265     when other
003266         display "MEXIT E  Unsupported exit type " exit-type "."
003267         set EXIT-RETURNCODE-FAILED to TRUE
003268
003269     end-evaluate
003270
004100     goback.
004252
004253/*****************************************************************
004254* exit-inexit -- Supply primary input to the compiler
004255******************************************************************
004256 exit-inexit.
004257
004262     evaluate TRUE
004263
004264         when EXIT-OPERATION-GET
004265             perform inexit-get
004266
004267         when EXIT-OPERATION-OPEN
004268             perform inexit-open
004269
004270         when EXIT-OPERATION-CLOSE
004271             perform inexit-close
004272
004273         when other
004274             display "MEXIT C  exit-inexit unsupported"
004276                 " operation " exit-operation "."
004277             set EXIT-RETURNCODE-FAILED to TRUE
004278
004279     end-evaluate
004280
004281     exit.
004282
004380/-----------------------------------------------------------------
004381* inexit-open -- Open a sequential ddname for reading
004382*
004383* inexit uses the first file-element.
004384*-----------------------------------------------------------------
004385 inexit-open.
004386
004387     display "MEXIT >  inexit-open"
004393     perform exit-open
004394
004398     perform 1 times
004399
004400         *> posit failure
004401         set EXIT-RETURNCODE-FAILED to TRUE
004402
004403         *> initialize file-element(1)
004404         set file-ptr(1) to NULL
004405         move ZERO       to file-count(1)
004407         move "SYSIN"    to file-library(1)
004408         move SPACES     to file-member(1)
004409
004410         *> build the DD filename
004411         string
004412             "//DD:"
004413             file-library(1) delimited by SPACE
004414             x'00' delimited by size
004415             into filename
004416         end-string
004417
004418         *> open the first element
004420         call "fopen" using
004421             filename
004422             by content z"rb"
004423         returning file-ptr(1)
004425
004426         if file-ptr(1) = NULL then
004428             display "MEXIT E  inexit-open of "
004429                 file-library(1) " failed, "
004432             exit perform
004433         end-if
               display "MEXIT I  inexit-open of"
                 file-library(1)
004434
004435         *> we're open for business
004436         set EXIT-RETURNCODE-OK to TRUE
004437
004438     end-perform
004439
004440     display "MEXIT <  inexit-open rc=" exit-returncode
004441
004442     exit.
004443
004444/-----------------------------------------------------------------
004445* inexit-get
004446*-----------------------------------------------------------------
004447 inexit-get.
004448
004449D    display "MEXIT >  inexit-get"
004450
004451     perform 1 times
004452
004453         *> posit failure
004454         set EXIT-RETURNCODE-FAILED to TRUE
004455
004456         *> read from our source file
004458         call "fread" using by value
004459             address of file-buffer(1)
004460             1 *> signal record I/O
004461             length of file-buffer(1)
004462             file-ptr(1)
004463         returning file-status
004465
004466         if file-status not = length of file-buffer(1) then

004468             *> do we have eof?
004470             call "feof" using by value
004471                 file-ptr(1)
004472             returning file-status
004474
004475             if file-status not = ZERO then
004476                 *> we have end of file
004477                 set EXIT-RETURNCODE-EOF to TRUE
004478                 exit perform
004479             end-if
004480
004481             *> we have some other kind of error
004483             display "MEXIT E  inexit-get of "
004484                 file-library(1) " failed, "
004487             exit perform
004488
004489         end-if
004490
004491         *> we've successfully read a record
004492         set EXIT-RETURNCODE-OK to TRUE
004493         add 1 to file-count(1)
004494         add 1 to serial-count
004495         move length of file-buffer(1) to exit-data-length
004496         set exit-data-buffer to address of file-buffer(1)
004497         move 1 to srcno; perform source-trace
004498
004499     end-perform
004500
004501D    display "MEXIT <  inexit-get rc=" exit-returncode
004502
004503     exit.
004504
004505/-----------------------------------------------------------------
004506* inexit-close
004507*-----------------------------------------------------------------
004508 inexit-close.
004509
004510     display "MEXIT >  inexit-close"
004511
004512     perform 1 times
004513
004514         *> posit failure
004515         set EXIT-RETURNCODE-FAILED to TRUE
004516
004517         *> close our file
004519         call "fclose" using by value
004520             file-ptr(1)
004521         returning file-status
004523         if file-status not = ZERO then
004525             display "MEXIT E  inexit-close of "
004526                 file-library(1) " failed, "
004529             exit perform
004530         end-if
004531
004532         *> we're now closed
004533         set EXIT-RETURNCODE-OK to TRUE
004534         display "MEXIT I  inexit-close " file-count(1)
004535             " records read from " file-library(1)
004536
004537     end-perform

           display "MEXIT I : Primary file close. Write out info here"
004542     display "MEXIT <  inexit-close rc=" exit-returncode
004543
004544     exit.
004545
004546/*****************************************************************
004547* exit-libexit
004548******************************************************************
004549 exit-libexit.
004550
004551     evaluate TRUE
004552
004553     when EXIT-OPERATION-GET
004554         perform libexit-get
004555
004556     when EXIT-OPERATION-FIND
004557         perform libexit-find
004558
004559     when EXIT-OPERATION-OPEN
004560         perform libexit-open
004561
004562     when EXIT-OPERATION-CLOSE
004563         perform libexit-close
004564
004565     when other
004566         display "MEXIT C  exit-libexit unsupported operation "
004567             exit-operation "."
004568         set EXIT-RETURNCODE-FAILED to TRUE
004569
004570     end-evaluate
004571
004572     exit.
004573
004574/-----------------------------------------------------------------
004575* libexit-open -- Check if the indicated library DD exists
004576*
004577* libexit-open is called only(?) the first time a specific library
004578* is seen.
004579*
004580* The code here just verifies that the library ddname exists and
004581* thereafter doesn't track the library in any way ie. there are
004582* currently no library-level resources to clean up at
004583* libexit-close.
004584*-----------------------------------------------------------------
004585 libexit-open.
004586
004587     display "MEXIT >  libexit-open " libno
004588         " " exit-system-library

004662         *> it all worked - this is a NOP for test where all src in HFS
004663         set EXIT-RETURNCODE-OK to TRUE
004664
004669     display "MEXIT <  libexit-open " libno
004670         " rc=" exit-returncode
004671
004672     exit.
004673
004674/-----------------------------------------------------------------
004675* libexit-find -- Find a copybook member and it open for reading
004676*
004677* But we may be returning here after a nested copybook is
004678* finished, and the file stack has just been popped. In this
004679* instance, and for this implementation,  we simply return OK
004680* because we have a positioned file-ptr from the stack.
004681*-----------------------------------------------------------------
004682 libexit-find.
004683
004684     display "MEXIT >  libexit-find " libno
004686         " " exit-member
004687         " of " exit-system-library
004688
004689     perform 1 times
004690
004691         *> posit failure
004692         set EXIT-RETURNCODE-FAILED to TRUE
004693
004717         *> check if we have stack space for this find/open
004718         if libno = FILE_STACK_SIZE then
004719             display "MEXIT C  libexit-find " libno
004720                 " stack overflow."
004721             exit perform
004722         end-if
004723
004724         *> push the stack
004725         add  1                   to libno
004726         set  file-ptr(libno)     to NULL
004727         move ZERO                to file-count(libno)
004729         move exit-system-library to file-library(libno)
004730         move exit-member         to file-member(libno)
004731
               *> build the 'file' name (not in a library)
               unstring file-member(libno) delimited by '"'
                 into empty raw-file
               string
                   "./"
                   raw-file delimited by SPACE
                   x'00' delimited by size
                   into filename
               end-string

004742
004743         *> do the open
               display "fopen <" filename ">"
004745         call "fopen" using
004746             filename
004747             by content z"rb"
004748         returning file-ptr(libno)
004750
004751         if file-ptr(libno) = NULL then
004760             display "MEXIT E  libexit-find " libno
004762                 " " file-member(libno)
004763                 " of " file-library(libno) " failed "

004768             *> discard the file-element
004769             subtract 1 from libno
004770
004771             exit perform
004772         end-if
004773
004774         *> we're good to go
004775         set EXIT-RETURNCODE-OK to TRUE
004776
004777     end-perform
004778
004779     display "MEXIT <  libexit-find " libno
004780         " rc=" exit-returncode
004781     exit.
004782
004783/-----------------------------------------------------------------
004784* libexit-get -- Get a line from a copybook
004785*
004786* We do actual repositioning here, that is to say we return the
004787* previously returned buffer.
004788*-----------------------------------------------------------------
004789 libexit-get.
004790
004791     *> entry trace only during debugging
004792     display "MEXIT >  libexit-get " libno
004794         " " file-member(libno)
004795         " of " file-library(libno)
004796
004797     perform 1 times
004798
004799         *> posit failure
004800         set EXIT-RETURNCODE-FAILED to TRUE
004801
004824         *> read from our source file
004826         call "fread" using by value
004827             address of file-buffer(libno)
004828             1 *> signal record I/O
004829             length of file-buffer(libno)
004830             file-ptr(libno)
004831         returning file-status
004833
004834         if file-status not = length of file-buffer(libno) then
004835
004836             *> do we have eof?
004838             call "feof" using by value
004839                 file-ptr(libno)
004840             returning file-status
004842
004843             if file-status not = ZERO then
004844                 *> we have end of file
004845                 perform libexit-close-member
004846                 set EXIT-RETURNCODE-EOF to TRUE
004847                 exit perform
                   else
004850               *> we have some kind of error
004852               display "MEXIT E  libexit-get " libno
004853                   " " file-member(libno)
004854                   " of " file-library(libno) " failed, "
004857               exit perform
004848             end-if
004859         end-if
004860
004861         *> we've successfully read a record
004862         set EXIT-RETURNCODE-OK to TRUE
004863         add 1 to file-count(libno)
004864         add 1 to serial-count
004865         move length of file-buffer(libno) to exit-data-length
004866         set exit-data-buffer to address of file-buffer(libno)
004867         move libno to srcno; perform source-trace
004868
004869     end-perform
004870
004871     *> exit trace only during debugging
004872     display "MEXIT <  libexit-get " libno
004873         " rc=" exit-returncode
004874
004875     exit.
004876
004877/-----------------------------------------------------------------
004878* libexit-close -- Final resource cleanup
004879*-----------------------------------------------------------------
004880 libexit-close.
004881
004882     display "MEXIT >  libexit-close " libno
004883
004884     *> pop stack, closing any files still open
004885     if not COPYBOOK_INACTIVE then
004886         display "MEXIT A  Remaining file-stack entries!"
004887         perform until COPYBOOK_INACTIVE
004888             perform libexit-close-member
004889         end-perform
004890     end-if
004891
004892     set EXIT-RETURNCODE-OK to TRUE
004893     display "MEXIT <  libexit-close " libno
004894         " rc=" exit-returncode
004895
004896     exit.
004897
004898/-----------------------------------------------------------------
004899* libexit-close-member -- Close the copybook member after EOF
004900*
004901* This is not an actual exit function, it's implied by libexit.get
004902* when EOF is reached.
004903*-----------------------------------------------------------------
004904 libexit-close-member.
004905
004906     display "MEXIT >  libexit-close-member " libno
004908         " " file-member(libno)
004909         " of " file-library(libno)
004910
004911     perform 1 times
004912
004913         *> posit failure
004914         set EXIT-RETURNCODE-FAILED to TRUE
004915
004916        *> assert if file-stack underflow
004917        if COPYBOOK_INACTIVE then
004918            display "MEXIT A  libexit-close-member " libno
004919                "file-stack is empty!"
004920            exit perform
004921        end-if
004922
004923         *> close our file
004925         call "fclose" using by value
004926             file-ptr(libno)
004927         returning file-status
004929
004930         if file-status not = ZERO then
004932             display "MEXIT E  libexit-close-member " libno
004933                 " " file-member(libno)
004934                 " of " file-library(libno) " failed, "
004937             exit perform
004938         end-if
004939
004940         *> we're now closed
004941         set EXIT-RETURNCODE-OK to TRUE
004942         display "MEXIT I  libexit-close-member " libno
004943             " " file-count(libno)
004944             " records read from " file-member(libno)
004945             " of " file-library(libno)
004946
004947     end-perform
004948
004949     *> pop the file-stack
004950     if not COPYBOOK_INACTIVE then
004951         subtract 1 from libno
004953     end-if
004954
004955     *> if there's still a copybook in progress then reposition
004960
004961     display "MEXIT <  libexit-close-member " libno
004962         " rc=" exit-returncode
004963
004964     exit.
004965
004966/-----------------------------------------------------------------
004967* exit-open -- Process the exit open
004968*
004969* Note that for the LIBEXIT, there are multiple opens, we only
004970* drive this code for the first of them.
004971*-----------------------------------------------------------------
004972 exit-open.
004973
004976     *> initial processing for either/both exits
004977     if EXIT_IS_INITIAL then
004978         move function when-compiled(1:14) to when-compiled-pic
004979         inspect when-compiled-str replacing all "," by ":"
004980         display "MEXIT I  Compiled " when-compiled-str "."
004981         set EXIT_IS_INITIAL to FALSE
004982     end-if
004983
004984     *> on (first?) open, data buffer points to the parm
004985     set address of parm to address of exit-data-buffer
004986
004987     evaluate TRUE
004988
004989     when EXIT-TYPE-INEXIT
004990         display "MEXIT I  inexit-open"
004991             " parm='" parm-data(1:parm-length) "'."
004992
004993     when EXIT-TYPE-LIBEXIT
004994         display "MEXIT I  libexit-open"
004995             " parm='" parm-data(1:parm-length) "'."
004996
004997     end-evaluate
004998
004999     exit.
005000
005014/-----------------------------------------------------------------
005015* exit-entry-trace -- Just in case debug logic
005016*-----------------------------------------------------------------
005017Dexit-entry-trace.
005018D
005019D    *> avoid heavy tracing
005020D    if EXIT-OPERATION-GET then exit paragraph
005021D
005022D    evaluate TRUE
005023D
005024D    when EXIT-TYPE-INEXIT
005025D        display "MEXIT.INEXIT >"
005026D        display "           exit-type: " exit-type
005027D        display "      exit-operation: " exit-operation
005028D        display "     exit-returncode: " exit-returncode
005029D        display "      exit-work-area: " exit-work-area
005030D            hex-of(address of exit-work-area)
005031D        display "    exit-data-length: " exit-data-length
005032D        display "    exit-data-buffer: "
005033D            hex-of(exit-data-buffer)
005034D
005035D    when EXIT-TYPE-LIBEXIT
005036D        display "MEXIT.LIBEXIT >"
005037D        display "              exit-type: " exit-type
005038D        display "         exit-operation: " exit-operation
005039D        display "        exit-returncode: " exit-returncode
005040D        display "         exit-work-area: " exit-work-area
005041D            hex-of(address of exit-work-area)
005042D        display "       exit-data-length: " exit-data-length
005043D        display "       exit-data-buffer: "
005044D            hex-of(exit-data-buffer)
005045D        display "    exit-system-library: '"
005046D            exit-system-library "'"
005047D        display "     exit-system-member: '"
005048D            exit-system-member "'"
005049D        display "           exit-library: '"
005050D            exit-library "'"
005051D        display "            exit-member: '"
005052D            exit-member "'"
005053D
005054D    when other
005055D        display "MEXIT E  Internal entry trace error."
005056D
005057D    end-evaluate
005058D
005059D    exit.
005060
005061/-----------------------------------------------------------------
005062* source-trace -- Show the source line returned to the compiler
005063*
005064* The caller MUST set srcno to 1 (inexit) or libno (libexit)
005065*-----------------------------------------------------------------
005066 source-trace.
005067
005068     display "MEXIT:  "
005069         file-library(srcno) " "
005070         file-member(srcno) " "
005071         file-count(srcno) " "
005072         serial-count
005073         ": " file-buffer(srcno)
005074
005075     exit.
005080
005100 end program "MEXIT".

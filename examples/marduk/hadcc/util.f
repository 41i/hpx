c
c 4 May 2011
c Steve Liebling
c Matt Anderson
c
cccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc
cc                                                                            cc
cc  find_bbox:                                                                cc
cc             Find bounding box for a binary [0,1] array.                    cc
cc             Note: finds bounding box within *existing* box                 cc
cc                   defined by [bbminx,bbmaxx][][].                          cc
cc             Likewise computes:                                             cc
cc                   sigi, sigj, sigk  ---Signature lines                     cc
cc                                                                            cc
cccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc
      subroutine find_bbox( flagarray,
     *                      sigi,        sigj,       sigk,
     *                      bbminx,      bbminy,     bbminz,
     *                      bbmaxx,      bbmaxy,     bbmaxz,
     *                      nx,          ny,         nz,
     *                      efficiency                           )
      implicit    none
      integer     nx,         ny,         nz,
     *            bbminx,     bbminy,     bbminz,
     *            bbmaxx,     bbmaxy,     bbmaxz
      real(kind=8)      flagarray(nx,ny,nz),
     *            sigi(nx),   sigj(ny),   sigk(nz),
     *            efficiency
      integer     i,          j,          k,
     *            bbminx_tmp, bbminy_tmp, bbminz_tmp,
     *            bbmaxx_tmp, bbmaxy_tmp, bbmaxz_tmp
      logical     ltrace
      parameter ( ltrace = .false. )

c     if (ltrace) then
c        write(*,*) 'find_bbox: bbminx, bbmaxx =',bbminx,bbmaxx
c        write(*,*) 'find_bbox: bbminy, bbmaxy =',bbminy,bbmaxy
c        write(*,*) 'find_bbox: bbminz, bbmaxz =',bbminz,bbmaxz
c        write(*,*) 'find_bbox: nx,ny,nz=',nx,ny,nz
c     end if
      !
      ! Zero out signature:
      !   (only need to zero where maximal bounding box exists:
      !
      do i = bbminx, bbmaxx
         sigi(i) = 0.d0
      end do
      do j = bbminy, bbmaxy
         sigj(j) = 0.d0
      end do
      do k = bbminz, bbmaxz
         sigk(k) = 0.d0
      end do

      !
      ! Define temporary bounding box:
      !
      bbminx_tmp = nx
      bbminy_tmp = ny
      bbminz_tmp = nz
      bbmaxx_tmp = 1
      bbmaxy_tmp = 1
      bbmaxz_tmp = 1

      efficiency = 0.d0

      do k = bbminz, bbmaxz
         do j = bbminy, bbmaxy
            do i = bbminx, bbmaxx
               if ( flagarray(i,j,k) .gt. 0 ) then
                  efficiency = efficiency + 1.d0
                  sigi(i)    = sigi(i) + 1.d0
                  sigj(j)    = sigj(j) + 1.d0
                  sigk(k)    = sigk(k) + 1.d0
                  if (i.gt.bbmaxx_tmp) bbmaxx_tmp = i
                  if (i.lt.bbminx_tmp) bbminx_tmp = i
                  if (j.gt.bbmaxy_tmp) bbmaxy_tmp = j
                  if (j.lt.bbminy_tmp) bbminy_tmp = j
                  if (k.gt.bbmaxz_tmp) bbmaxz_tmp = k
                  if (k.lt.bbminz_tmp) bbminz_tmp = k
               end if
            end do
         end do
c        if (ltrace) write(*,*) k, sigk(k)
      end do

      !
      ! replace input bbox with  the bounding box found here:
      !
      bbminx = bbminx_tmp
      bbminy = bbminy_tmp
      bbminz = bbminz_tmp
      bbmaxx = bbmaxx_tmp
      bbmaxy = bbmaxy_tmp
      bbmaxz = bbmaxz_tmp

c     if (ltrace) write(*,*) 'find_bbox: numpoints = ',efficiency

      efficiency = efficiency / (bbmaxx - bbminx + 1)
     *                        / (bbmaxy - bbminy + 1)
     *                        / (bbmaxz - bbminz + 1)

      if (ltrace) then
         write(*,*) 'find_bbox: efficiency     = ',efficiency
         write(*,*) 'find_bbox: bbminx, bbmaxx =',bbminx,bbmaxx
         write(*,*) 'find_bbox: bbminy, bbmaxy =',bbminy,bbmaxy
         write(*,*) 'find_bbox: bbminz, bbmaxz =',bbminz,bbmaxz
         do i = bbminx, bbmaxx
            write(*,*) i, sigi(i)
         end do
      end if

      return
      end     ! END: find_bbox

cccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc
cc                                                                            cc
cc  compute_disallowed:                                                       cc
cc             Compute signatures and Laplacians for disallowed points        cc
cc             in a given cluster.                                            cc
cc                                                                            cc
cccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc
      subroutine compute_disallowed( flagarray,
     *                      sigi,        sigj,       sigk,
     *                      lapi,        lapj,       lapk,
     *                      bbminx,      bbminy,     bbminz,
     *                      bbmaxx,      bbmaxy,     bbmaxz,
     *                      nx,          ny,         nz,
     *                      disallowedlogical                    )
      implicit    none
      integer     nx,         ny,         nz,
     *            bbminx,     bbminy,     bbminz,
     *            bbmaxx,     bbmaxy,     bbmaxz
      real(kind=8)      flagarray(nx,ny,nz),
     *            sigi(nx),   sigj(ny),   sigk(nz),
     *            lapi(nx),   lapj(ny),   lapk(nz)
      logical     disallowedlogical
      integer     i,          j,          k
      integer     lengthx,    lengthy,    lengthz
      logical     ltrace
      parameter ( ltrace = .false. )

      if (ltrace) then
         write(*,*) 'compute_disallowed: bbminx/maxx: ',bbminx,bbmaxx
         write(*,*) 'compute_disallowed: bbminy/maxy: ',bbminy,bbmaxy
         write(*,*) 'compute_disallowed: bbminz/maxz: ',bbminz,bbmaxz
         write(*,*) 'compute_disallowed: nx/y/z:      ',nx,ny,nz
      end if

      !
      ! Zero out signature:
      !   (only need to zero where bounding box exists:
      !
      do i = bbminx, bbmaxx
         sigi(i) = 0.d0
      end do
      do j = bbminy, bbmaxy
         sigj(j) = 0.d0
      end do
      do k = bbminz, bbmaxz
         sigk(k) = 0.d0
      end do


      disallowedlogical = .false.

      do k = bbminz, bbmaxz
         do j = bbminy, bbmaxy
            do i = bbminx, bbmaxx
               if ( flagarray(i,j,k) .lt. 0 ) then
                  disallowedlogical = .true.
                  sigi(i)    = sigi(i) + 1.d0
                  sigj(j)    = sigj(j) + 1.d0
                  sigk(k)    = sigk(k) + 1.d0
               end if
            end do
         end do
      end do

      !
      ! Only compute lapi/j/k if disallowed points are here:
      !
      if (disallowedlogical) then
         lengthx = bbmaxx-bbminx+1
         lengthy = bbmaxy-bbminy+1
         lengthz = bbmaxz-bbminz+1
         !write(*,*) lengthx,lengthy, lengthz
         call  compute_lap(lapi(bbminx), sigi(bbminx), lengthx )
         call  compute_lap(lapj(bbminy), sigj(bbminy), lengthy )
         call  compute_lap(lapk(bbminz), sigk(bbminz), lengthz )
      end if

      if (ltrace) then
         write(*,*) 'compute_disallowed: disallowedlogical: ',
     *                                   disallowedlogical
      end if

      return
      end     ! END: compute_disallowed

cccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc
cc                                                                            cc
cc  find_inflect:                                                             cc
cc          finds largest inflection point in 1d vector                       cc
cc          Used for clustering ala Berger and Rigtousis '91.                 cc
cc                                                                            cc
cccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc
      subroutine find_inflect( u1, max, max_i, nx)
      implicit    none
      integer     max_i, nx
      real(kind=8)      max, u1(nx)
      integer     i
      real(kind=8)      tmp, tmp2
      logical     ltrace
      parameter ( ltrace      = .false. )


      max     = 0.d0
      tmp     = u1(1)

      do i = 2, nx
         if ( u1(i)*tmp .lt. 0.d0 ) then
            !
            ! If opposite sign, then an inflection point:
            !
            tmp2            = abs(u1(i)-tmp)
            if (ltrace) write(*,*) ' Inflection pt found: ',i,tmp2
            if (tmp2 .gt. max) then
               if (ltrace) write(*,*) ' New max'
               max   = tmp2
               max_i = i - 1
            end if
         end if
         tmp = u1(i)
      end do


      return
      end      ! END: find_inflect

cccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc
cc                                                                            cc
cc  load_scal1d:                                                              cc
cc                                                                            cc
cccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc
      subroutine load_scal1d( field, scalar, nx)
      implicit   none
      integer    nx
      real(kind=8)     field(nx), scalar
      integer    i

            do i = 1, nx
               field(i) = scalar
            end do

      return
      end    ! END: load_scal1d
cccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc
cc                                                                            cc
cc  find_min1d:                                                               cc
cc          finds minimum of 1d vector                                        cc
cc          If multiple regions of the same minimum value,                    cc
cc          it finds the one with the largest span.                           cc
cc             min   = minimum value found                                    cc
cc             min_i = index of minimum where first found                     cc
cc             span  = number of successive points at this minimum            cc
cc                                                                            cc
cccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc
      subroutine find_min1d( u1, min, min_i, span, nx)
      implicit    none
      integer     min_i,   span, nx
      real(kind=8)      u1(nx),  min
      integer     min_i_tmp,   span_tmp
      real(kind=8)      min_tmp
      integer     i
      !real(kind=8)      LARGENUMBER
      !parameter ( LARGENUMBER = 9.d98 )
      include       'largesmall.inc'
      logical     within_span, within_span_tmp
      logical     ltrace
      parameter ( ltrace      = .false. )

      if (ltrace) then
         write(*,*) 'find_min1d: nx     = ',nx
         write(*,*) 'find_min1d: u1(1)  = ',u1(1)
         write(*,*) 'find_min1d: u1(nx) = ',u1(nx)
      end if

      if (nx.le.0) then
            min   = LARGENUMBER
            min_i = 0
            span  = 0
            return
      end if

      min     = LARGENUMBER
      min_tmp = LARGENUMBER + 1

      do i = 1, nx
         if (ltrace) write(*,*) '      i, u1(i)=',i,u1(i)
         if ( u1(i) .lt. min ) then
            min             = u1(i)
            min_i           = i
            span            = 1
            within_span     = .true.
            within_span_tmp = .false.
         else if ( (u1(i).eq.min) .and. within_span) then
            span            = span + 1
         else if (        (u1(i).eq.min)
     *             .and. .not.within_span
     *             .and. .not.within_span_tmp) then
            min_tmp         = min
            min_i_tmp       = i
            span_tmp        = 1
            within_span_tmp = .true.
         else if ( (u1(i).eq.min) .and. within_span_tmp) then
            span_tmp        = span_tmp + 1
         else                !   (u1(i) .ne. min)
            within_span     = .false.
            within_span_tmp = .false.
            if ( (min .eq. min_tmp) .and. (span_tmp.gt.span) ) then
               span  = span_tmp
               min_i = min_i_tmp
            end if
         end if
      end do

      if (ltrace) then
         write(*,*) 'find_min1d: min   = ', min
         write(*,*) 'find_min1d: min_i = ', min_i
         write(*,*) 'find_min1d: span  = ', span
      end if

      return
      end      ! END: find_min1d
cccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc
cc                                                                            cc
cc  compute_lap:                                                              cc
cc          computes Laplacian terms from signature arrays...                 cc
cc          Used for clustering ala Berger and Rigtousis '91.                 cc
cc                                                                            cc
cccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc
      subroutine compute_lap( lap, sig, nx)
      implicit    none
      integer     nx
      real(kind=8)      lap(nx), sig(nx)
      integer     i

      lap(1)  = 0.d0
      lap(nx) = 0.d0
      do i = 2, nx-1
         lap(i) = 2.d0*sig(i) - sig(i-1) - sig(i+1)
      end do

      return
      end      ! END: compute_lap


cccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc
cc                                                                            cc
cc    grow_bbox:                                                              cc
cc              Expand bboxes to include a ghostwidth but                     cc
cc              be careful not to:                                            cc
cc                  1) include any disallowed points                          cc
cc                  2) go past the bounds of the grid                         cc
cc                                                                            cc
cccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc
      subroutine grow_bbox( flagarray, value, width, bmini,bmaxi,
     *                      bminj,bmaxj,bmink,bmaxk, nx,ny,nz)
      implicit     none
      integer      nx, ny, nz, width, bmini,bmaxi,
     *             bminj,bmaxj, bmink,bmaxk
      real(kind=8) flagarray(nx,ny,nz), value
      !
      integer     i, j, k, num
      logical      double_equal
      external     double_equal
      logical      allowed
      logical      ltrace
      parameter (  ltrace = .false. )

      if (ltrace) then
         write(*,*) 'grow_bbox: value:     ',value
         write(*,*) 'grow_bbox: width:     ',width
         write(*,*) 'grow_bbox: bmin/axi:  ',bmini,bmaxi
         write(*,*) 'grow_bbox: bmin/axj:  ',bminj,bmaxj
         write(*,*) 'grow_bbox: bmin/axk:  ',bmink,bmaxk
         write(*,*) 'grow_bbox: nx/y/z:    ',nx,ny,nz
      end if

      if (.false.  ) then
         !
         ! No disallowed points to worry about:
         !
         bmini = bmini - width
         bmaxi = bmaxi + width
         if (bmini .lt.  1) bmini =  1
         if (bmaxi .gt. nx) bmaxi = nx
         !
         bminj = bminj - width
         bmaxj = bmaxj + width
         if (bminj .lt.  1) bminj =  1
         if (bmaxj .gt. ny) bmaxj = ny
         !
         bmink = bmink - width
         bmaxk = bmaxk + width
         if (bmink .lt.  1) bmink =  1
         if (bmaxk .gt. nz) bmaxk = nz
         !
      else
         !
         ! Try to expand out "width" points
         ! but do not include disallowed points
         !
         ! X min:
         !
         num     = 0
   10    i       = bmini - num - 1
         if (i.lt. 1) goto 15
         allowed = .true.
         do k = bmink, bmaxk
            do j = bminj, bmaxj
               if (double_equal(flagarray(i,j,k),value)) then
                  allowed = .false.
               end if
            end do
         end do
         if (allowed) then
            ! Increase the number by which we can grow box:
            num = num + 1
            if (num .lt. width) goto 10
         end if
   15    continue
         bmini = bmini - num
         if (ltrace) write(*,*) 'grow_bbox: Changing bmini: ',bmini,num
         !
         ! X max:
         !
         num     = 0
   20    i       = bmaxi + num + 1
         if (i.gt.nx) goto 25
         allowed = .true.
         do k = bmink, bmaxk
            do j = bminj, bmaxj
               if (double_equal(flagarray(i,j,k),value)) then
                  allowed = .false.
               end if
            end do
         end do
         if (allowed) then
            ! Increase the number by which we can grow box:
            num = num + 1
            if (num .lt. width) goto 20
         end if
   25    continue
         bmaxi = bmaxi + num
         if (ltrace) write(*,*) 'grow_bbox: Changing bmaxi: ',bmaxi,num
         !
         ! Y min:
         !
         num     = 0
   30    j       = bminj - num - 1
         if (j.lt. 1) goto 35
         allowed = .true.
         do k = bmink, bmaxk
            do i = bmini, bmaxi
               if (double_equal(flagarray(i,j,k),value)) then
                  allowed = .false.
               end if
            end do
         end do
         if (allowed) then
            ! Increase the number by which we can grow box:
            num = num + 1
            if (num .lt. width) goto 30
         end if
   35    continue
         bminj = bminj - num
         if (ltrace) write(*,*) 'grow_bbox: Changing bminj: ',bminj,num
         !  
         ! Y max:
         !
         num     = 0
   40    j       = bmaxj + num + 1
         if (j.gt.ny) goto 45
         allowed = .true.
         do k = bmink, bmaxk
            do i = bmini, bmaxi
               if (double_equal(flagarray(i,j,k),value)) then
                  allowed = .false.
               end if
            end do
         end do
         if (allowed) then
            ! Increase the number by which we can grow box:
            num = num + 1
            if (num .lt. width) goto 40
         end if
   45    continue
         bmaxj = bmaxj + num
         if (ltrace) write(*,*) 'grow_bbox: Changing bmaxj: ',bmaxj,num
         !  
         ! Z min:
         !
         num     = 0
   50    k       = bmink - num - 1
         if (k.lt. 1) goto 55
         allowed = .true.
         do j = bminj, bmaxj
            do i = bmini, bmaxi
               if (double_equal(flagarray(i,j,k),value)) then
                  allowed = .false.
               end if
            end do
         end do
         if (allowed) then
            ! Increase the number by which we can grow box:
            num = num + 1
            if (num .lt. width) goto 50
         end if
   55    continue
         bmink = bmink - num
         if (ltrace) write(*,*) 'grow_bbox: Changing bmink: ',bmink,num
         !
         ! Z max:
         !
         num     = 0
   60    k       = bmaxk + num + 1
         if (k.gt.nz) goto 65
         allowed = .true.
         do j = bminj, bmaxj
            do i = bmini, bmaxi
               if (double_equal(flagarray(i,j,k),value)) then
                  allowed = .false.
               end if
            end do
         end do
         if (allowed) then
            ! Increase the number by which we can grow box:
            num = num + 1
            if (num .lt. width) goto 60
         end if
   65    continue
         bmaxk = bmaxk + num
         if (ltrace) write(*,*) 'grow_bbox: Changing bmaxk: ',bmaxk,num
         !
      end if

      return
      end      ! END: grow_bbox

cccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc
cc                                                                            cc
cc  double_equal:                                                             cc
cc                                                                            cc
cccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc
      logical function double_equal( first, second )
      implicit    none
      real(kind=8)      first, second
      include       'largesmall.inc'
      !real(kind=8)      SMALLNUMBER
      !parameter       ( SMALLNUMBER = 1.0d-14)
      logical           ltrace
      parameter       ( ltrace      = .false. )

      double_equal = abs(first-second) .lt. SMALLNUMBER

      if (ltrace) then
         write(*,*)'double_equal:first,second',first,second,double_equal
         write(*,*)'double_equal:diff ',abs(first-second),SMALLNUMBER
         write(*,*)'double_equal:log',abs(first-second).lt.SMALLNUMBER
      end if

      return
      end    ! END: double_equal

cccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc
cc                                                                            cc
cc  load_scal_mult3d:                                                         cc
cc                    Load scalar*fieldin to fieldout.                        cc
cc                                                                            cc
cccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc
      subroutine load_scal_mult3d( fieldout, fieldin, scalar, nx,ny,nz)
      implicit   none
      integer    nx, ny, nz
      real(kind=8)     fieldout(nx,ny,nz), fieldin(nx,ny,nz), scalar
      integer    i,j,k

      do k = 1, nz
         do j = 1, ny
            do i = 1, nx
               fieldout(i,j,k) = scalar * fieldin(i,j,k)
            end do
         end do
      end do

      return
      end    ! END: load_scal_mult3d


cccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc
cc                                                                            cc
cc  level_makeflag:                                                           cc
cc                 Make flag array for level:                                 cc
cc                     (1) Initialize flag array to DISALLOW                  cc
cc                     (2) If error exceeds threshold, flag                   cc
cc                     (3) Everywhere a refined grid exists, flag             cc
cc                     (4) If any points are flagged, flag where masked       cc
cc                     (5) Add a buffer region of flagged points              cc
cc                     (6) Disallow ghostregion of level                      cc
cc                                                                            cc
cccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc
      subroutine level_makeflag( flag, error, level,
     *                           minx,miny,minz, h,nx,ny,nz,
     *                           ethreshold)
      implicit none
      integer       nx,ny,nz, level
      real(kind=8)  minx,miny,minz, h, ethreshold
      real(kind=8)  flag(nx,ny,nz), error(nx,ny,nz)
      include      'largesmall.inc'
      integer       gi, numpoints
      integer       i, j, k, l
      integer       mini,maxi, minj,maxj, mink,maxk
      integer       bi,ei, bj,ej, bk,ek
      integer       resolution, izero,jzero, kzero
      logical       inmaskedregion, unresolved, cancellevel
      logical       childlevelexists
      real(kind=8)  r1_2, x,y,z
      logical       double_equal
      external      double_equal
      real(kind=8)  myl2norm3d, errornorm
      external      myl2norm3d

      !
      ! Minimum number of grid points in any direction for a masked region:
      !    (in units of number of grid points)
      !    (set to 0 to turn off this "feature")
      !
      integer       MINRESOLUTION
      parameter (   MINRESOLUTION = 13 )
      !parameter (   MINRESOLUTION = 8 )
      !
      ! Width of buffer region around a masked region to refine:
      !    (in units of number of grid points)
      !
      integer       MASKBUFFER
      parameter (   MASKBUFFER = 5 )

      logical      ltrace
      parameter (  ltrace  = .false.)
      logical      ltrace2
      parameter (  ltrace2 = .false.)

      real(kind=8)   FLAG_REFINE
      parameter    ( FLAG_REFINE   =  1.d0 )
      !     ...points NOT to be refined
      real(kind=8)   FLAG_NOREFINE
      parameter    ( FLAG_NOREFINE =  0.d0 )
      !     ...points which do not exist on the level
      real(kind=8)   FLAG_DISALLOW
      parameter    ( FLAG_DISALLOW = -1.d0 )

      if (ltrace) write(*,*) 'level_makeflag: Initializing flag: ',level

      call load_scal3d(flag, FLAG_DISALLOW, nx,ny,nz )

      !
      ! Keep track of bounding box for FLAG_REFINE points:
      !
      numpoints = 0

      !
      ! If certain bad conditions, then we will not
      ! create this level
      !
      cancellevel      = .false.
      childlevelexists = .false.

      mini = nx
      minj = ny
      mink = nz
      maxi = 1
      maxj = 1
      maxk = 1

      if (ltrace) write(*,*) 'level_makeflag: Flagging where high error'
      do k = 1, nz
         do j = 1, ny
            do i = 1, nx
               if (error(i,j,k) .ge. ethreshold-SMALLNUMBER) then
                  flag(i,j,k) = FLAG_REFINE
                  numpoints   = numpoints + 1
                  if(.false.)write(*,*)'level_makeflag: Flagged: ',
     *                                                i,j,k,error(i,j,k)
                  if (i.lt.mini) mini = i
                  if (j.lt.minj) minj = j
                  if (k.lt.mink) mink = k
                  if (i.gt.maxi) maxi = i
                  if (j.gt.maxj) maxj = j
                  if (k.gt.maxk) maxk = k
               else if (error(i,j,k) .ge. 0-SMALLNUMBER) then
                  flag(i,j,k) = FLAG_NOREFINE
               else if (ltrace2) then
                  if (NINT(error(i,j,k)).ne.-1) then
                     write(*,*) 'level_makeflag: Disallowed pt: ',
     *                                         error(i,j,k),i,j,k
                  end if
               end if
            end do
         end do
      end do
      if (ltrace2) then
         write(*,*) 'level_makeflag:   ethreshold: ',ethreshold
         write(*,*) 'level_makeflag:   numpoints flagged: ',numpoints
         write(*,*) 'level_makeflag:   mini/j/k:',mini,minj,mink
         write(*,*) 'level_makeflag:   maxi/j/k:',maxi,maxj,maxk
         write(*,*) 'level_makeflag:   nx/y/z:  ',nx,ny,nz
      end if

      if (ltrace) write(*,*) 'level_makeflag: Done on level ',level

      return
      end        ! END: level_makeflag

cccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc
cc                                                                            cc
cc  load_scal3d:                                                              cc
cc                                                                            cc
cccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc
      subroutine load_scal3d( field, scalar, nx, ny, nz)
      implicit   none
      integer    nx, ny, nz
      real(kind=8)     field(nx,ny,nz), scalar
      integer    i,j,k

      do k = 1, nz
         do j = 1, ny
            do i = 1, nx
               field(i,j,k) = scalar
            end do
         end do
      end do

      return
      end    ! END: load_scal3d

cccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc
cc                                                                            cc
cc  mat_buffer:                                                               cc
cc              Takes a flag array, and flags points withing "buffer" pts     cc
cc              NB: if a point is disallowed, it will not get buffered/flaggedcc
cc                                                                            cc
cccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc
      subroutine mat_buffer( flag, work, buffer, nx, ny, nz)
      implicit   none
      integer    nx, ny, nz, buffer
      real(kind=8)     flag(nx,ny,nz), work(nx,ny,nz)
      integer    i,j,k, ii,jj,kk

      !
      ! Copy flag to work array:
      !
      call mat_copy3d( flag, work, nx,ny,nz)

      !
      ! Remake flag array with buffer:
      !
      do k = 1, nz
         do j = 1, ny
            do i = 1, nx
               if ( NINT(work(i,j,k)) .eq. 1) then
                  do kk = k-buffer, k+buffer
                  do jj = j-buffer, j+buffer
                  do ii = i-buffer, i+buffer
                     if (ii.ge.1 .and. ii.le.nx .and.
     *                   jj.ge.1 .and. jj.le.ny .and.
     *                   kk.ge.1 .and. kk.le.nz ) then
!    *                   NINT(work(ii,jj,kk)).ge.0) then
                         if (NINT(work(ii,jj,kk)).ge.0)
     *                      flag(ii,jj,kk) = work(i,j,k)
                     end if
                  end do
                  end do
                  end do
               end if
            end do
         end do
      end do

      return
      end    ! END: mat_buffer

cccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc
cc                                                                            cc
cc  mat_copy1d:                                                               cc
cc                                                                            cc
cccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc
      subroutine mat_copy1d( field_n, field_np1, nx)
      implicit   none
      integer    nx
      real(kind=8)     field_n(nx), field_np1(nx)
      integer    i

            do i = 1, nx
               field_np1(i) = field_n(i)
            end do

      return
      end    ! END: mat_copy1d

cccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc
cc                                                                            cc
cc  mat_copy3d:                                                               cc
cc                                                                            cc
cccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc
      subroutine mat_copy3d( field_n, field_np1, nx, ny, nz)
      implicit   none
      integer    nx, ny, nz
      real(kind=8)     field_n(nx,ny,nz), field_np1(nx,ny,nz)
      integer    i,j,k

      do k = 1, nz
         do j = 1, ny
            do i = 1, nx
               field_np1(i,j,k) = field_n(i,j,k)
            end do
         end do
      end do

      return
      end    ! END: mat_copy3d




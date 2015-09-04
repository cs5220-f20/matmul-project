!
! Illustrate a Fortran 2003 style C-Fortran interface
! It has been more than a decade since the standard --
! most compilers should support it at this point.
!
      subroutine square_dgemm(M, A, B, C) bind(C)
      use, intrinsic :: iso_c_binding
      implicit none
      integer (c_int), value :: M
      real (c_double), intent (inout) :: A(*)
      real (c_double), intent (inout) :: B(*)
      real (c_double), intent (inout) :: C(*)
      integer :: MM
      double precision :: ONE
      double precision :: ZERO
      ONE = 1.0
      ZERO = 0.0
      MM = M
      call dgemm("N", "N", MM, MM, MM, ONE, A, MM, B, MM, ZERO, C, MM)
      end subroutine square_dgemm

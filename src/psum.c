#include "data.table.h"

// like base::p{max,min}, but for sum
SEXP psum(SEXP x, SEXP narmArg) {
  if (!isNewList(x)) error(_("Internal error: x must be a list"));
  if (!isLogical(narmArg) || LENGTH(narmArg)!=1 || LOGICAL(narmArg)[0]==NA_LOGICAL) error(_("na.rm must be TRUE or FALSE"));

  int J=LENGTH(x);
  if (J == 0) {
    error(_("Empty input"));
  } else if (J == 1) {
    // na.rm doesn't matter -- input --> output
    return duplicate(VECTOR_ELT(x, 0));
  }
  SEXPTYPE outtype = INTSXP;
  int n = -1;
  for (int j=0; j<J; j++) {
    SEXP xj=VECTOR_ELT(x, j);
    switch(TYPEOF(xj)) {
    case INTSXP:
      break;
    case REALSXP:
      outtype = REALSXP;
      break;
    default:
      error(_("Only numeric inputs are supported for psum"));
    }
    if (n >= 0) {
      int nj = LENGTH(xj);
      if (n == 1 && nj > 1) {
        n = nj; // singleton comes first, vector comes later [psum(1, 1:4)]
      } else if (nj != 1 && nj != n) {
        error(_("Inconsistent input lengths -- first found %d, but %d element has length %d. Only singletons will be recycled."), n, j+1, nj);
      }
    } else { // initialize
      n = LENGTH(xj);
    }
  }

  // TODO: support int->double conversion
  SEXP out = PROTECT(allocVector(outtype, n));
  if (n == 0) {
    UNPROTECT(1);
    return(out);
  }
  if (LOGICAL(narmArg)[0]) {
    // initialize to NA to facilitate all-NA rows --> NA output
    writeNA(out, 0, n);
    if (outtype == INTSXP) {
      int *outp = INTEGER(out);
      for (int j=0; j<J; j++) {
        SEXP xj = VECTOR_ELT(x, j);
        int nj = LENGTH(xj);
        int *xjp = INTEGER(xj);
        for (int i=0; i<n; i++) {
          int xi = nj == 1 ? 0 : i; // recycling for singletons
          if (xjp[xi] != NA_INTEGER) {
            outp[i] = outp[i] == NA_INTEGER ? xjp[xi] : outp[i] + xjp[xi];
          } // else remain as NA
        }
      }
    } else { // REALSXP; special handling depending on whether each input is int/numeric
      double *outp = REAL(out);
      for (int j=0; j<J; j++) {
        SEXP xj = VECTOR_ELT(x, j);
        int nj = LENGTH(xj);
        switch (TYPEOF(xj)) {
        case INTSXP: {
          int *xjp = INTEGER(xj);
          for (int i=0; i<n; i++) {
            int xi = nj == 1 ? 0 : i;
            if (xjp[xi] != NA_INTEGER) {
              outp[i] = ISNAN(outp[i]) ? (double)xjp[xi] : outp[i] + (double)xjp[xi];
            }
          }
        } break;
        case REALSXP: {
          double *xjp = REAL(xj);
          for (int i=0; i<n; i++) {
            int xi = nj == 1 ? 0 : i;
            if (!ISNAN(xjp[xi])) {
              outp[i] = ISNAN(outp[i]) ? xjp[xi] : outp[i] + xjp[xi];
            }
          }
        } break;
        default:
          error(_("Internal error: should have caught non-INTSXP/REALSXP input by now"));
        }
      }
    }
  } else { // na.rm=FALSE
    if (outtype == INTSXP) {
      int *outp = INTEGER(out);
      SEXP xj0 = VECTOR_ELT(x, 0);
      int nj0 = LENGTH(xj0);
      int *xj0p = INTEGER(xj0);
      for (int i=0; i<n; i++) outp[i] = xj0p[nj0 == 1 ? 0 : i];
      for (int j=1; j<J; j++) { // J>=2 by now
        SEXP xj = VECTOR_ELT(x, j);
        int nj = LENGTH(xj);
        int *xjp = INTEGER(xj);
        for (int i=0; i<n; i++) {
          int xi = nj == 1 ? 0 : i;
          outp[i] = outp[i] == NA_INTEGER || xjp[xi] == NA_INTEGER ? NA_INTEGER : outp[i] + xjp[xi];
        }
      }
    } else { // REALSXP
      double *outp = REAL(out);
      SEXP xj0 = VECTOR_ELT(x, 0);
      int nj0 = LENGTH(xj0);
      if (TYPEOF(xj0) == INTSXP) {
        int *xj0p = INTEGER(xj0);
        for (int i=0; i<n; i++) outp[i] = (double)xj0p[nj0 == 1 ? 0 : i];
      } else { // x[[1]] must be REALSXP in current logic
        double *xj0p = REAL(xj0);
        for (int i=0; i<n; i++) outp[i] = xj0p[nj0 == 1 ? 0 : i];
      }
      for (int j=1; j<J; j++) {
        SEXP xj=VECTOR_ELT(x, j);
        int nj = LENGTH(xj);
        switch (TYPEOF(xj)) {
        case INTSXP: {
          int *xjp = INTEGER(xj);
          for (int i=0; i<n; i++) {
            int xi = nj == 1 ? 0 : i;
            outp[i] = outp[i] == NA_REAL || xjp[xi] == NA_INTEGER ? NA_REAL : outp[i] + (double)xjp[xi];
          }
        } break;
        case REALSXP: {
          double *xjp = REAL(xj);
          for (int i=0; i<n; i++) {
            int xi = nj == 1 ? 0 : i;
            outp[i] = outp[i] == NA_REAL || xjp[xi] == NA_REAL ? NA_REAL : outp[i] + xjp[xi];
          }
        } break;
        default:
          error(_("Internal error: should have caught non-INTSXP/REALSXP input by now"));
        }
      }
    }
  }

  UNPROTECT(1);
  return out;
}

SEXP pprod(SEXP x, SEXP narmArg) {
  if (!isNewList(x)) error(_("Internal error: x must be a list"));
  if (!isLogical(narmArg) || LENGTH(narmArg)!=1 || LOGICAL(narmArg)[0]==NA_LOGICAL) error(_("na.rm must be TRUE or FALSE"));

  int J=LENGTH(x);
  if (J == 0) {
    error(_("Empty input"));
  } else if (J == 1) {
    // na.rm doesn't matter -- input --> output
    return duplicate(VECTOR_ELT(x, 0));
  }
  SEXPTYPE outtype = INTSXP;
  int n = -1;
  for (int j=0; j<J; j++) {
    SEXP xj=VECTOR_ELT(x, j);
    switch(TYPEOF(xj)) {
    case INTSXP:
      break;
    case REALSXP:
      outtype = REALSXP;
      break;
    default:
      error(_("Only numeric inputs are supported for pprod"));
    }
    if (n >= 0) {
      if (n != LENGTH(xj)) {
        error(_("Inconsistent input lengths -- first found %d, but %d element has length %d"), n, j+1, LENGTH(xj));
      }
    } else { // initialize
      n = LENGTH(xj);
    }
  }


  // TODO: support int->double conversion
  SEXP out = PROTECT(allocVector(outtype, n));
  if (n == 0) {
    UNPROTECT(1);
    return(out);
  }
  if (LOGICAL(narmArg)[0]) {
    // initialize to NA to facilitate all-NA rows --> NA output
    writeNA(out, 0, n);
    if (outtype == INTSXP) {
      int *outp = INTEGER(out);
      for (int j=0; j<J; j++) {
        int *xjp = INTEGER(VECTOR_ELT(x, j));
        for (int i=0; i<n; i++) {
          if (xjp[i] != NA_INTEGER) {
            outp[i] = outp[i] == NA_INTEGER ? xjp[i] : outp[i] * xjp[i];
          } // else remain as NA
        }
      }
    } else { // REALSXP; special handling depending on whether each input is int/numeric
      double *outp = REAL(out);
      for (int j=0; j<J; j++) {
        SEXP xj = VECTOR_ELT(x, j);
        switch (TYPEOF(xj)) {
        case INTSXP: {
          int *xjp = INTEGER(xj);
          for (int i=0; i<n; i++) {
            if (xjp[i] != NA_INTEGER) {
              outp[i] = ISNAN(outp[i]) ? (double)xjp[i] : outp[i] * (double)xjp[i];
            }
          }
        } break;
        case REALSXP: {
          double *xjp = REAL(xj);
          for (int i=0; i<n; i++) {
            if (!ISNAN(xjp[i])) {
              outp[i] = ISNAN(outp[i]) ? xjp[i] : outp[i] * xjp[i];
            }
          }
        } break;
        default:
          error(_("Internal error: should have caught non-INTSXP/REALSXP input by now"));
        }
      }
    }
  } else { // na.rm=FALSE
    if (outtype == INTSXP) {
      int *outp = INTEGER(out);
      int *xj0p = INTEGER(VECTOR_ELT(x, 0));
      for (int i=0; i<n; i++) outp[i] = xj0p[i];
      for (int j=1; j<J; j++) { // J>=2 by now
        int *xjp = INTEGER(VECTOR_ELT(x, j));
        for (int i=0; i<n; i++) {
          outp[i] = outp[i] == NA_INTEGER || xjp[i] == NA_INTEGER ? NA_INTEGER : outp[i] * xjp[i];
        }
      }
    } else { // REALSXP
      double *outp = REAL(out);
      if (TYPEOF(VECTOR_ELT(x, 0)) == INTSXP) {
        int *xj0p = INTEGER(VECTOR_ELT(x, 0));
        for (int i=0; i<n; i++) outp[i] = (double)xj0p[i];
      } else { // x[[1]] must be REALSXP in current logic
        double *xj0p = REAL(VECTOR_ELT(x, 0));
        for (int i=0; i<n; i++) outp[i] = xj0p[i];
      }
      for (int j=1; j<J; j++) {
        SEXP xj=VECTOR_ELT(x, j);
        switch (TYPEOF(xj)) {
        case INTSXP: {
          int *xjp = INTEGER(xj);
          for (int i=0; i<n; i++) {
            outp[i] = outp[i] == NA_REAL || xjp[i] == NA_INTEGER ? NA_REAL : outp[i] * (double)xjp[i];
          }
        } break;
        case REALSXP: {
          double *xjp = REAL(xj);
          for (int i=0; i<n; i++) {
            outp[i] = outp[i] == NA_REAL || xjp[i] == NA_REAL ? NA_REAL : outp[i] * xjp[i];
          }
        } break;
        default:
          error(_("Internal error: should have caught non-INTSXP/REALSXP input by now"));
        }
      }
    }
  }

  UNPROTECT(1);
  return out;
}

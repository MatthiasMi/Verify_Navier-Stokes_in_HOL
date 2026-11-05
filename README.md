# Verification

Recently, I ran a few verification experiments involving theorem-proving systems such as **Lean, HOL, ... **.

## HOL Proof Trace of Navier Stokes Conjecture

Along the way, I stumbled upon [DJB's microblog-entry](https://microblog.cr.yp.to/1793704234/index.html#1793704234).
I transferred the code into a standalone Git repository, investigated it, and needed to apply a few compatibility fixes.
Just to make it ready for further experiments, I had to change (`bool` → `booltype`) to get it compiling with modern C compilers.

Now I can

```sh
make holtrace-verify
```

and confirm that `navierstokes.50.zst`, the HOL Proof Trace of Navier Stokes Conjecture, verifies:

```sh
unzstd < navierstokes.50.zst | ./holtrace-verify
verified 1681498687 bytes, 206915187 lines, 174847 types, 98717497 terms, 54001524 infs, 54001522 thms, 23 typedefs
```

### git diff

Renames the local variable 'bool' to 'booltype' in term_new_eq() to avoid a naming conflict with the C99 bool type.

```sh
diff --git a/holtrace-verify.c b/holtrace-verify.c
--- a/holtrace-verify.c
+++ b/holtrace-verify.c
@@ -876,6 +876,6 @@
-  type_ptr bool = type_new_bool();
-  type_ptr typetobool = type_new_fun(type,bool);
+  type_ptr booltype = type_new_bool();
+  type_ptr typetobool = type_new_fun(type,booltype);
   type_ptr typetotypetobool = type_new_fun(type,typetobool);
   term_ptr eq = term_new_const(typetotypetobool,str_const_equalssign);
   term_ptr eqleft = term_new_eval(typetobool,eq,left);
-  return term_new_eval(bool,eqleft,right);
+  return term_new_eval(booltype,eqleft,right);
```

### Large files on GitHub

Trying to keep this repository self-contained, I ran into the following:

```text
! [remote rejected] main -> main (pre-receive hook declined)

remote: error: File navierstokes.50.zst is 260.57 MB; this exceeds GitHub's file size limit of 100.00 MB
remote: warning: GH001: Large files detected. You may want to try Git Large File Storage - https://git-lfs.github.com.
```

Well, I did not particularly care to investigate Git LFS, so I simply split the file into three parts instead

```text
navierstokes.50.zst.part.0
navierstokes.50.zst.part.1
navierstokes.50.zst.part.2
```

thus getting around the hard 100 MB limit. Technically, we're good - although GitHub still has something to say:

```text
remote: warning: File navierstokes.50.zst.part.2 is 86.86 MB;
this is larger than GitHub's recommended maximum file size of 50.00 MB.
```

To reconstruct the original file after checkout

```bash
cat navierstokes.50.zst.part.{0,1,2} > navierstokes.50.zst
```

and verify its integrity

```bash
sha256sum -c navierstokes.50.zst.sha256sum
```

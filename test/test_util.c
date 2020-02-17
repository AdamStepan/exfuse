#include "../src/inode.h"
#include "../src/util.h"
#include <glib.h>

void test_has_perm_other() {

    struct ex_inode ino;

    ino.uid = 0;
    ino.gid = 0;
    ino.mode = 0;

    assert(!ex_inode_has_perm(&ino, EX_READ, 1000, 1000));
    assert(!ex_inode_has_perm(&ino, EX_WRITE, 1000, 1000));
    assert(!ex_inode_has_perm(&ino, EX_EXECUTE, 1000, 1000));

    ino.mode = S_IRWXO;

    assert(ex_inode_has_perm(&ino, EX_READ, 1000, 1000));
    assert(ex_inode_has_perm(&ino, EX_WRITE, 1000, 1000));
    assert(ex_inode_has_perm(&ino, EX_EXECUTE, 1000, 1000));
}

void test_has_perm_user_and_group() {
    struct ex_inode ino;

    ino.uid = 0;
    ino.gid = 0;
    ino.mode = S_IRWXO | S_IRGRP;

    // we should be able to read, but nothing else
    assert(ex_inode_has_perm(&ino, EX_READ, 0, 0));
    assert(!ex_inode_has_perm(&ino, EX_WRITE, 0, 0));
    assert(!ex_inode_has_perm(&ino, EX_EXECUTE, 0, 0));

    ino.mode = S_IRWXO | S_IXUSR | S_IRGRP;

    // we should be able to execute only, the group is different
    assert(!ex_inode_has_perm(&ino, EX_READ, 1000, 0));
    assert(!ex_inode_has_perm(&ino, EX_WRITE, 1000, 0));
    assert(ex_inode_has_perm(&ino, EX_EXECUTE, 1000, 0));
}


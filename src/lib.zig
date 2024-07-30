const std = @import("std");
pub const c = @import("c.zig");
pub fn strchr(str: []u8, ch: u8) ?[]u8 {
    for (str, 0..) |*p, i| {
        if (p.* == ch) {
            return str[i..];
        }
    }
    return null;
}

pub fn strrchr(str: []u8, ch: u8) ?[]u8 {
    var i: i32 = @intCast(str.len - 1);
    while (i >= 0) : (i -= 1) {
        if (str[@intCast(i)] == ch) {
            return str[@intCast(i)..];
        }
    }
    return null;
}

const NMARKS = 32;

// line operations
pub const LOpt = extern struct {
    // inserted text
    ins: *c_char,
    // deleted text
    del: *c_char,
    // modification location
    pos: c_int,
    n_ins: c_int,
    n_del: c_int,
    // cursor line offset
    pos_off: c_int,
    // operation number
    seq: c_int,
    // saved marks
    mark: *c_int,
    mark_off: *c_int,
};

export fn lopt_done(lo: *LOpt) void {
    std.c.free(lo.ins);
    std.c.free(lo.del);
    std.c.free(lo.mark);
    std.c.free(lo.mark_off);
}

// line buffers
pub const LBuf = extern struct {
    // mark lines
    mark: [NMARKS]c_int,
    // mark line offsets
    mark_off: [NMARKS]c_int,
    // buffer lines
    ln: [*][*]c_char,
    // line global mark
    ln_glob: [*]c_char,
    // number of lines in ln[]
    ln_n: c_int,
    // size of ln[]
    ln_sz: c_int,
    // current operation sequence
    useq: c_int,
    // buffer history
    hist: [*]LOpt,
    // size of hist[]
    hist_sz: c_int,
    // current history head in hist[]
    hist_n: c_int,
    // current undo head in hist[]
    hist_u: c_int,
    // useq for lbuf_saved()
    useq_zero: c_int,
    // useq before hist[]
    useq_last: c_int,

    fn globget(self: @This(), pos: i32, dep: i32) bool {
        const o = self.ln_glob[@intCast(pos)] & @as(c_char, 1) << @intCast(dep);
        self.ln_glob[@intCast(pos)] &= ~(@as(c_char, 1) << @intCast(dep));
        return o > 0;
    }

    fn seq(self: @This()) i32 {
        return if (self.hist_u > 0) self.hist[@intCast(self.hist_u - 1)].seq else self.useq_last;
    }
};

extern fn ex_lbuf() *LBuf;

// mark buffer as saved and, if clear, clear the undo history
export fn lbuf_saved(lb: *LBuf, clear: c_int) void {
    if (clear != 0) {
        for (0..@intCast(lb.hist_n)) |i|
            lopt_done(&lb.hist[i]);
        lb.hist_n = 0;
        lb.hist_u = 0;
        lb.useq_last = lb.useq;
    }
    lb.useq_zero = lb.seq();
    _ = lbuf_modified(ex_lbuf());
}

// was the file modified since the last lbuf_modreset()
export fn lbuf_modified(lb: *LBuf) c_int {
    lb.useq += 1;
    return if (lb.seq() != lb.useq_zero) 1 else 0;
}

// mark the line for ex global command
export fn lbuf_globset(lb: *LBuf, pos: c_int, dep: c_int) void {
    lb.ln_glob[@intCast(pos)] |= @as(c_char, 1) << @intCast(dep);
}

// return and clear ex global command mark
export fn lbuf_globget(lb: *LBuf, pos: c_int, dep: c_int) c_int {
    return if (lb.globget(pos, dep)) 1 else 0;
}

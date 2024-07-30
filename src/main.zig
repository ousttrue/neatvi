const std = @import("std");
const c = @import("c.zig");
const lib = @import("lib");

fn get_prog() []const u8 {
    if (lib.strchr(std.mem.span(std.os.argv[0]), '/')) |_| {
        if (lib.strrchr(std.mem.span(std.os.argv[0]), '/')) |r| {
            return r[1..];
        } else {
            unreachable;
        }
    } else {
        return std.mem.span(std.os.argv[0]);
    }
}

pub fn main() void {
    // int i;
    const prog = get_prog();
    c.xvis = if (!std.mem.eql(u8, "ex", prog) and !std.mem.eql(u8, "neatex", prog)) 1 else 0;
    var i: usize = 1;
    while (i < std.os.argv.len and std.os.argv[i][0] == '-') : (i += 1) {
        if (std.os.argv[i][1] == 's')
            c.xled = 0;
        if (std.os.argv[i][1] == 'e')
            c.xvis = 0;
        if (std.os.argv[i][1] == 'v')
            c.xvis = 1;
        if (std.os.argv[i][1] == 'h') {
            std.debug.print("usage: {s} [options] [file...]\n\n", .{std.os.argv[0]});
            std.debug.print("options:\n", .{});
            std.debug.print("  -v    start in vi mode\n", .{});
            std.debug.print("  -e    start in ex mode\n", .{});
            std.debug.print("  -s    silent mode (for ex mode only)\n", .{});
            return;
        }
    }
    c.dir_init();
    c.syn_init();
    _ = c.tag_init();
    if (c.ex_init(@ptrCast(&std.os.argv[i])) == 0) {
        if (c.xled != 0 or c.xvis != 0)
            c.term_init();
        if (c.xvis != 0) {
            c.vi();
        } else {
            c.ex();
        }
        if (c.xled != 0 or c.xvis != 0)
            c.term_done();
        c.ex_done();
    }
    std.c.free(c.w_path);
    c.reg_done();
    c.syn_done();
    c.dir_done();
    c.tag_done();
}

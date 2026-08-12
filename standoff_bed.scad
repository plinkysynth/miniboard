// Flat mounting bed with two rows of M3 standoffs.
// Units are millimetres.

// to make an x-direction cutaway.
// my bambu labs printer can only do 250mm at a time
// so i print it in two parts

// part 1
//clip_min = [250, -1, -1]; // -1, -1, -1
//clip_max = [510, 160, 20]; // 510, 160, 20

// part 2
//clip_min = [-1, -1, -1]; // -1, -1, -1
//clip_max = [250, 160, 20]; // 510, 160, 20

// or just the whole thing in one go
clip_min = [-1, -1, -1]; // -1, -1, -1
clip_max = [510, 160, 20]; // 510, 160, 20


$fn = 64;

bed_x = 500;
bed_y = 150;
bed_thickness = 4;

pillar_diameter = 8;
default_pillar_height = 0;
pcb_pillar_h = default_pillar_height + 5.1;

// M3 socket-head bolt allowances. Adjust for your bolt/printing process.
bolt_hole_diameter = 3.2;
bolt_head_recess_diameter = 6.0;
bolt_head_recess_depth = 2;

epsilon = 0.01;

x_offset = (bed_x - 481.55) / 2;
x_coordinates = [0, 69.72, 137.07, 302.27, 481.55];
y_coordinates = [20, 96];

mb_w = 29.845;
mb_h = 20.828;
mb_x = 30;
mb_y = 5;

// Add further entries as [x, y, pillar_height].
// The supplied x coordinates are centred by x_offset here.
risers = [
    for (y = y_coordinates, x = x_coordinates)
        [x + x_offset, y, default_pillar_height]
];

// Positive pillar geometry. Its M3 bore is removed by riser_cutout() below.
module riser(x, y, pillar_height) {
    translate([x, y, bed_thickness])
        cylinder(d = pillar_diameter, h = pillar_height);
}

// M3 clearance bore through the bed and pillar, plus the bolt-head recess
// opening from the bottom of the bed.
module riser_cutout(x, y, pillar_height) {
    translate([x, y, -epsilon])
        cylinder(
            d = bolt_hole_diameter,
            h = bed_thickness + pillar_height + 2 * epsilon
        );

    translate([x, y, -epsilon])
        cylinder(
            d = bolt_head_recess_diameter,
            h = bolt_head_recess_depth + epsilon
        );
}

module riser_cutout2(x, y, pillar_height) {
    translate([x, y, -epsilon])
        cylinder(
            d = bolt_hole_diameter,
            h = bed_thickness + pillar_height + 2 * epsilon
        );

    translate([x, y, -epsilon])

        cylinder(
            h = 4,
               r1=8/2,r2=4/2
        );
}

module complete_bed() {
    difference() {
        union() {
            cube([bed_x, bed_y, bed_thickness]);

            for (r = risers)
                riser(r[0], r[1], r[2]);
            
            riser(mb_x, mb_y, pcb_pillar_h);
            riser(mb_x + mb_w, mb_y + mb_h, pcb_pillar_h);
        }

        for (r = risers)
            riser_cutout2(r[0], r[1], r[2]);
        
        riser_cutout(mb_x, mb_y, pcb_pillar_h);
        riser_cutout(mb_x+mb_w, mb_y+mb_h, pcb_pillar_h);
        
        
    }
}


mirror([1,0,0])
intersection() {
    complete_bed();
    translate(clip_min)
        cube(clip_max - clip_min);
}

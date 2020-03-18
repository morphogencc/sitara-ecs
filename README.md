# sitara-ecs
Entity Component System for Cinder, used by Sitara Systems LLC

Heavily influenced by (and some code outright borrowed from!) Sosolimited's [Entity Component Samples](https://github.com/sosolimited/Entity-Component-Samples).

## Requirements

### EntityX
EntityX is included as a submodule.  You'll need to run CMake and configure the CMake files to copy files into the appropriate directories.  You do *not* need to build the project.

### Bullet3
Bullet3 is included as a static library.

## Usage

### Transform System
- World and Local Transforms with Parent/Child Relationships

### Rigid Body System
- Bullet Physics for rigid body collisions

### Geometry System
- Drawing wireframe and solid shapes
- Shader Support

### Logic System
- Logical Layers
- Logical States

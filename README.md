# MoonFlower

A statically-typed bytecode interpreter (and eventually language) for use with games.

## Status

It can add integers.

## Why?

Because Lua is cool but I want destructors.

## Goals

- Similar in usage and syntax to Lua
- Fully portable, inline assembly and JIT are not allowed
- Performance should be as close to PUC Lua as possible (faster?)
- Statically typed, no tagged types
- No garbage collector, deterministic allocations
- Destructors
- Direct "linking" with C++ types and functions
- Opinionated module system
- Modules can be unloaded and reloaded (not while referenced)

## Requirements

Bison and Flex.

function(AddTargetExecutable targetName targetMain)
  add_executable(
      ${PROJECT_NAME}_${targetName}.elf
      ${targetMain}
  )
  target_link_libraries(
    ${PROJECT_NAME}_${targetName}.elf
    PRIVATE
    stm32f4_hal
  )
  # Make a .bin from the .elf
  add_custom_target(${PROJECT_NAME}_${targetName}.bin ALL
      arm-none-eabi-objcopy -O binary ${PROJECT_NAME}_${targetName}.elf ${PROJECT_NAME}_${targetName}.bin
      DEPENDS ${PROJECT_NAME}_${targetName}.elf
  )
endfunction()

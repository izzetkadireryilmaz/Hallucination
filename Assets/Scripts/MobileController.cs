using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.UI;

public class MobileController : MonoBehaviour
{
    private CharacterController characterController;

    [SerializeField] float _speed = 4f, _Xspeed = 6f, _jump = 1f, _gravity = -9.8f;
    [SerializeField] Transform groundCheck;
    [SerializeField] float groundDistance = 0.3f;
    [SerializeField] LayerMask groundLayerMask;
    [SerializeField] Joystick movementJoystik;
    Vector3 _velocity;
    bool isGrounded;

    private void Start()
    {
        characterController = GetComponent<CharacterController>();
    }

    private void Update()
    {
        isGrounded = Physics.CheckSphere(groundCheck.position, groundDistance, groundLayerMask);

        if (isGrounded && _velocity.y < 0)
        {
            _velocity.y = -2;
        }

        float x = movementJoystik.Horizontal;
        float z = movementJoystik.Vertical;

        Vector3 move = transform.right * x + transform.forward * z;

        if (z > 0.8f || Input.GetKey(KeyCode.LeftShift))
        {
            characterController.Move(move * _Xspeed * Time.deltaTime);
        }
        else
        {
            characterController.Move(move * _speed * Time.deltaTime);
        }

        _velocity.y += _gravity * Time.deltaTime;
        characterController.Move(_velocity * Time.deltaTime);
    }
}

using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public class PlayerMovement : MonoBehaviour
{
    private static PlayerMovement instance;
    CharacterController controller;
    public float walkingSpeed;
    public float runningSpeed;
    public float gravity;
    private Vector3 velocity;

    private void Start()
    {
        controller = GetComponent<CharacterController>();
    }
    void Awake()
    {
        if (instance == null)
        {
            instance = this;
            DontDestroyOnLoad(gameObject);
        }
        else
        {
            Destroy(gameObject);
        }
    }

    void Update()
    {
        // Ýleri ve yatay hareketleri alýyoruz.
        float horizontal = Input.GetAxis("Horizontal");
        float vertical = Input.GetAxis("Vertical");

        Vector3 move = transform.forward * vertical + transform.right * horizontal;

        // Shift tuþu ile hareket hýzýmýzý arttýrýyoruz.
        if (Input.GetKey(KeyCode.LeftShift) || Input.GetKey(KeyCode.RightShift))
        {
            controller.Move(move * runningSpeed * Time.deltaTime);
        }
        else
        {
            controller.Move(move * walkingSpeed * Time.deltaTime);
        }

        // Karakterin yere olan temasýný kontrol ediyoruz.
        if (controller.isGrounded)
        {
            velocity.y = -2f; // Karakter yere temas ediyorsa bile küçük bir kuvvet uygulayarak yere tam oturmayý saðlýyoruz.
        }
        else
        {
            velocity.y += gravity * Time.deltaTime; // Karakter yere temas etmiyorsa belirlediðimiz yer çekimi etkisini uyguluyoruz.
        }

        controller.Move(velocity * Time.deltaTime); // Yer çekimi uygulanmasýný saðlýyoruz.
    }

}
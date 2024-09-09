using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.Audio;

public class TriggerController : MonoBehaviour
{
    public AudioSource GameSource;
    public AudioSource audioSource;
    public AudioSource CamSource;
    public GameObject targetObjectWithAnimator;
    public GameObject targetObjectWithAnimator2;
    public GameObject targetObjectWithAnimator3;
    public Animator animator;

    private void OnTriggerEnter(Collider other)
    {
        if (other.gameObject.CompareTag("Trigger"))
        {
            Animator animator = targetObjectWithAnimator.GetComponent<Animator>();
            if (animator != null)
            {
                Debug.Log("asd");
                animator.SetBool("Sürünüyor", true);

            }

            if (!audioSource.isPlaying)
            {
                audioSource.Play();
                // 15 saniye sonra müziði durdur
                StartCoroutine(StopMusicAfterDelay(15f));
            }
        }
        if (other.gameObject.CompareTag("Trigger2"))
        {
            Animator animator = targetObjectWithAnimator2.GetComponent<Animator>();
            if (animator != null)
            {
                Debug.Log("asd");
                animator.SetBool("Sürünüyor", true);
            }

            if (!audioSource.isPlaying)
            {
                audioSource.Play();
                // 15 saniye sonra müziði durdur
                StartCoroutine(StopMusicAfterDelay(15f));
            }
        }
        if (other.gameObject.CompareTag("Final"))
        {
            Animator animator = targetObjectWithAnimator3.GetComponent<Animator>();
            if (animator != null)
            {
                Debug.Log("asd");
                animator.SetBool("Final", true);
                CamSource.Play();
            }
            if (!audioSource.isPlaying)
            {
                CamSource.Play();
                // 15 saniye sonra müziði durdur
                StartCoroutine(StopCamAfterDelay(15f));
            }
        }
        GameSource.volume = 0.3f;
    }

    IEnumerator StopMusicAfterDelay(float delay)
    {
        yield return new WaitForSeconds(delay);
        GameSource.volume = 1f;
        audioSource.Stop();
    }
    IEnumerator StopCamAfterDelay(float delay)
    {
        yield return new WaitForSeconds(delay);
        GameSource.volume = 1f;
        audioSource.Stop();
    }
}
